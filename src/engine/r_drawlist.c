// Emacs style mode select   -*- C -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 2007-2012 Samuel Villarreal
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
//-----------------------------------------------------------------------------
//
// DESCRIPTION: Vertex draw lists.
// Stores geometry info produced by R_RenderBSPNode into a list for optimal rendering
//
//-----------------------------------------------------------------------------

#include <stdlib.h>

#include "r_drawlist.h"
#include "doomdef.h"
#include "doomstat.h"
#include "r_main.h"
#include "r_things.h"
#include "gl_texture.h"
#include "gl_main.h"
#include "i_system.h"
#include "z_zone.h"
#include "dgl.h"

vtx_t drawVertex[MAXDLDRAWCOUNT];

static float envcolor[4] = { 0, 0, 0, 0 };

drawlist_t drawlist[NUMDRAWLISTS];

CVAR_EXTERNAL(r_texturecombiner);
CVAR_EXTERNAL(r_objectFilter);

extern int GL_WorldTextureIsTranslucent(int texnum);
extern float fviewx, fviewy, fviewz;
extern leaf_t* leafs;
extern int vertCount;

// -----------------------------------------------------------------------------
// DL_AddVertexList
// -----------------------------------------------------------------------------

vtxlist_t* DL_AddVertexList(drawlist_t* dl) {
    vtxlist_t* list;

    list = &dl->list[dl->index];

    if (list == &dl->list[dl->max - 1]) {
        // add a new list to the array
        dl->max++;

        // allocate array
        dl->list =
            (vtxlist_t*)Z_Realloc(dl->list,
                dl->max * sizeof(vtxlist_t), PU_LEVEL, NULL);

        dmemset(&dl->list[dl->max - 1], 0, sizeof(vtxlist_t));

        list = &dl->list[dl->index];
    }

    list->flags = 0;
    list->texid = 0;
    list->params = 0;

    return &dl->list[dl->index++];
}

// Sorting
static int SortDrawList(const void* a, const void* b) {
    const vtxlist_t* xa = (const vtxlist_t*)a;
    const vtxlist_t* xb = (const vtxlist_t*)b;
    return xb->texid - xa->texid;
}

static int SortSprites(const void* a, const void* b) {
    const visspritelist_t* xa = (const visspritelist_t*)((const vtxlist_t*)a)->data;
    const visspritelist_t* xb = (const visspritelist_t*)((const vtxlist_t*)b)->data;
    return xb->dist - xa->dist;
}

static inline int is_translucent_entry(int tag, const vtxlist_t* item) {
    if (tag == DLT_SPRITE)
        return 0;

    if (tag != DLT_WALL) {
        if (item->flags & DLF_WATER1)
            return 1;
    }

    const int world_tex = (item->texid & 0xffff);
    return GL_WorldTextureIsTranslucent(world_tex) ? 1 : 0;
}

static double item_distance(int tag, const vtxlist_t* item) {
    if (tag == DLT_WALL) {
        const seg_t* seg = (const seg_t*)item->data;
        const double cx = ((double)(seg->v1->x + seg->v2->x) * 0.5) / 65536.0;
        const double cy = ((double)(seg->v1->y + seg->v2->y) * 0.5) / 65536.0;
        const double dx = cx - (double)fviewx;
        const double dy = cy - (double)fviewy;
        return dx * dx + dy * dy;
    }
    else {
        const subsector_t* ss = (const subsector_t*)item->data;
        const int a = 0;
        const int b = (ss->numleafs > 1) ? (ss->numleafs >> 1) : 0;
        const leaf_t* la = &leafs[ss->leaf + a];
        const leaf_t* lb = &leafs[ss->leaf + b];
        const double cx = ((double)(la->vertex->x + lb->vertex->x) * 0.5) / 65536.0;
        const double cy = ((double)(la->vertex->y + lb->vertex->y) * 0.5) / 65536.0;
        const double dx = cx - (double)fviewx;
        const double dy = cy - (double)fviewy;
        return dx * dx + dy * dy;
    }
}

// -----------------------------------------------------------------------------
// DL_ProcessDrawList
// -----------------------------------------------------------------------------

void DL_ProcessDrawList(int tag, boolean(*procfunc)(vtxlist_t*, int*)) {
    drawlist_t* dl;
    int i;
    int drawcount = 0;
    vtxlist_t* head;
    vtxlist_t* tail;
    boolean checkNightmare = false;
    boolean obj_nearest_bypass = false;

    if (tag < 0 || tag >= NUMDRAWLISTS) {
        return;
    }

    dl = &drawlist[tag];

    if (dl->max > 0) {
        int palette = 0;

        if (tag != DLT_SPRITE) {
            qsort(dl->list, dl->index, sizeof(vtxlist_t), SortDrawList);
        }
        else if (dl->index >= 2) {
            qsort(dl->list, dl->index, sizeof(vtxlist_t), SortSprites);
        }

        head = dl->list;
        tail = &dl->list[dl->index];

        /* ------------------- OPAQUE / ALPHA-TESTED ------------------- */
        for (i = 0; i < dl->index; i++, head++) {
            vtxlist_t* rover;

            if (tag != DLT_SPRITE && is_translucent_entry(tag, head)) {
                continue;
            }

            obj_nearest_bypass = false;

            // break if no data found in list
            if (!head->data) {
                break;
            }

            if (drawcount >= MAXDLDRAWCOUNT) {
                I_Error("DL_ProcessDrawList: Draw overflow by %i, tag=%i", dl->index, tag);
            }

            if (procfunc) {
                if (!procfunc(head, &drawcount)) {
                    continue;
                }
            }

            rover = head + 1;

            if (tag != DLT_SPRITE) {
                if (rover != tail) {
                    if (head->texid == rover->texid && head->params == rover->params) {
                        continue;
                    }
                }
            }

            if (tag == DLT_SPRITE) {
                unsigned int flags = ((visspritelist_t*)head->data)->spr->flags;
                unsigned int __packed = (unsigned int)head->texid;
                palette = (int)((__packed >> 24) & 0xFF);
                head->texid = (int)(__packed & 0xFFFF);
                GL_BindSpriteTexture(head->texid, palette);

                // Non-monster objects obey r_objectFilter
                if (!(flags & MF_COUNTKILL) && ((int)r_objectFilter.value > 0)) {
                    I_ShaderUnBind();
                    dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                    dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                    obj_nearest_bypass = true;
                }

                // change blend states for nightmare things
                if ((checkNightmare ^ (flags & MF_NIGHTMARE))) {
                    if (!checkNightmare && (flags & MF_NIGHTMARE)) {
                        dglBlendFunc(GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR);
                        checkNightmare ^= 1;
                    }
                    else if (checkNightmare && !(flags & MF_NIGHTMARE)) {
                        dglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                        checkNightmare ^= 1;
                    }
                }
            }
            else {
                head->texid = (head->texid & 0xffff);
                GL_BindWorldTexture(head->texid, 0, 0);
            }

            // non sprite textures must repeat or mirrored-repeat
            if (tag == DLT_WALL) {
                dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                    head->flags & DLF_MIRRORS ? GL_MIRRORED_REPEAT : GL_REPEAT);
                dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                    head->flags & DLF_MIRRORT ? GL_MIRRORED_REPEAT : GL_REPEAT);
            }

            if (r_texturecombiner.value > 0) {
                envcolor[0] = envcolor[1] = envcolor[2] = ((float)head->params / 255.0f);
                GL_SetEnvColor(envcolor);
                dglTexCombColorf(GL_TEXTURE0_ARB, envcolor, GL_ADD);
            }
            else {
                int l = (head->params >> 1);
                GL_UpdateEnvTexture(D_RGBA(l, l, l, 0xff));
            }

            dglDrawGeometry(drawcount, drawVertex);

            if (obj_nearest_bypass) {
                I_ShaderBind();
                obj_nearest_bypass = false;
            }

            // count vertex size
            if (devparm) {
                vertCount += drawcount;
            }

            drawcount = 0;
            head->data = NULL; // consumed
        }

        /* ------------------- TRANSLUCENT (BACK-TO-FRONT) ------------- */
        if (tag == DLT_SPRITE) {
            for (i = 0; i < dl->index; i++) {
                vtxlist_t* rover;

                head = &dl->list[i];

                obj_nearest_bypass = false;

                if (!head->data) {
                    break;
                }

                if (drawcount >= MAXDLDRAWCOUNT) {
                    I_Error("DL_ProcessDrawList: Draw overflow by %i, tag=%i", dl->index, tag);
                }

                if (procfunc) {
                    if (!procfunc(head, &drawcount)) {
                        continue;
                    }
                }

                rover = head + 1;

                if (tag != DLT_SPRITE) {
                    if (rover != tail) {
                        if (head->texid == rover->texid && head->params == rover->params) {
                            continue;
                        }
                    }
                }

                if (tag == DLT_SPRITE) {
                    unsigned int flags = ((visspritelist_t*)head->data)->spr->flags;

                    unsigned int __packed = (unsigned int)head->texid;
                    palette = (int)((__packed >> 24) & 0xFF);
                    head->texid = (int)(__packed & 0xFFFF);
                    GL_BindSpriteTexture(head->texid, palette);

                    if (!(flags & MF_COUNTKILL) && ((int)r_objectFilter.value > 0)) {
                        I_ShaderUnBind();
                        dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                        dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                        obj_nearest_bypass = true;
                    }

                    if ((checkNightmare ^ (flags & MF_NIGHTMARE))) {
                        if (!checkNightmare && (flags & MF_NIGHTMARE)) {
                            dglBlendFunc(GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR);
                            checkNightmare ^= 1;
                        }
                        else if (checkNightmare && !(flags & MF_NIGHTMARE)) {
                            dglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                            checkNightmare ^= 1;
                        }
                    }
                }

                if (r_texturecombiner.value > 0) {
                    envcolor[0] = envcolor[1] = envcolor[2] = ((float)head->params / 255.0f);
                    GL_SetEnvColor(envcolor);
                    dglTexCombColorf(GL_TEXTURE0_ARB, envcolor, GL_ADD);
                }
                else {
                    int l = (head->params >> 1);
                    GL_UpdateEnvTexture(D_RGBA(l, l, l, 0xff));
                }

                dglDrawGeometry(drawcount, drawVertex);

                if (obj_nearest_bypass) {
                    I_ShaderBind();
                    obj_nearest_bypass = false;
                }

                if (devparm) {
                    vertCount += drawcount;
                }

                drawcount = 0;
                head->data = NULL;
            }
        }
        else {
            int count = 0;
            vtxlist_t** plist = (vtxlist_t**)Z_Calloc(sizeof(vtxlist_t*) * dl->index, PU_LEVEL, 0);
            for (i = 0; i < dl->index; ++i) {
                if (dl->list[i].data) {
                    plist[count++] = &dl->list[i];
                }
            }

            if (count > 1) {
                int j, k;
                for (j = 1; j < count; ++j) {
                    vtxlist_t* tmp = plist[j];
                    double dtmp = item_distance(tag, tmp);
                    k = j - 1;
                    while (k >= 0 && item_distance(tag, plist[k]) < dtmp) {
                        plist[k + 1] = plist[k];
                        --k;
                    }
                    plist[k + 1] = tmp;
                }
            }

            for (i = 0; i < count; ++i) {
                head = plist[i];
                obj_nearest_bypass = false;

                if (!head->data) {
                    continue;
                }

                if (drawcount >= MAXDLDRAWCOUNT) {
                    I_Error("DL_ProcessDrawList: Draw overflow by %i, tag=%i", dl->index, tag);
                }

                if (procfunc) {
                    if (!procfunc(head, &drawcount)) {
                        continue;
                    }
                }

                // Bind world texture for this translucent entry
                head->texid = (head->texid & 0xffff);
                GL_BindWorldTexture(head->texid, 0, 0);

                if (tag == DLT_WALL) {
                    dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                        head->flags & DLF_MIRRORS ? GL_MIRRORED_REPEAT : GL_REPEAT);
                    dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                        head->flags & DLF_MIRRORT ? GL_MIRRORED_REPEAT : GL_REPEAT);

                    dglDepthMask(GL_FALSE);
                    dglDepthFunc(GL_LESS);
                    GL_SetState(GLSTATE_BLEND, 1);
                    dglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                    dglDisable(GL_ALPHA_TEST);
                }
                else {
                    dglEnable(GL_POLYGON_OFFSET_FILL);
                    dglPolygonOffset(4.0f, 8.0f);
                    dglDepthMask(GL_FALSE);
                    dglDepthFunc(GL_LESS);
                    GL_SetState(GLSTATE_BLEND, 1);
                    dglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                    dglDisable(GL_ALPHA_TEST);
                }

                if (r_texturecombiner.value > 0) {
                    envcolor[0] = envcolor[1] = envcolor[2] = ((float)head->params / 255.0f);
                    GL_SetEnvColor(envcolor);
                    dglTexCombColorf(GL_TEXTURE0_ARB, envcolor, GL_ADD);
                }
                else {
                    int l = (head->params >> 1);
                    GL_UpdateEnvTexture(D_RGBA(l, l, l, 0xff));
                }

                dglDrawGeometry(drawcount, drawVertex);

                if (tag == DLT_WALL) {
                    dglEnable(GL_ALPHA_TEST);
                    GL_SetState(GLSTATE_BLEND, 0);
                    dglDepthFunc(GL_LESS);
                    dglDepthMask(GL_TRUE);
                }
                else {
                    dglEnable(GL_ALPHA_TEST);
                    GL_SetState(GLSTATE_BLEND, 0);
                    dglDepthFunc(GL_LESS);
                    dglDepthMask(GL_TRUE);
                    dglDisable(GL_POLYGON_OFFSET_FILL);
                }

                if (devparm) {
                    vertCount += drawcount;
                }

                drawcount = 0;
                head->data = NULL;
            }

            Z_Free(plist);
        }
    }
}

// -----------------------------------------------------------------------------
// DL_GetDrawListSize
// -----------------------------------------------------------------------------

int DL_GetDrawListSize(int tag) {
    int i;

    for (i = 0; i < NUMDRAWLISTS; i++) {
        drawlist_t* dl;

        if (i != tag) {
            continue;
        }

        dl = &drawlist[i];
        return dl->max * sizeof(vtxlist_t);
    }

    return 0;
}

// -----------------------------------------------------------------------------
// DL_BeginDrawList
// -----------------------------------------------------------------------------

void DL_BeginDrawList(boolean t, boolean a) {
    dglSetVertex(drawVertex);

    GL_SetTextureUnit(0, t);

    if (a) {
        dglTexCombColorf(GL_TEXTURE0_ARB, envcolor, GL_ADD);
    }
}

// -----------------------------------------------------------------------------
// DL_Init
// -----------------------------------------------------------------------------

void DL_Init(void) {
    drawlist_t* dl;
    int i;

    for (i = 0; i < NUMDRAWLISTS; i++) {
        dl = &drawlist[i];

        dl->index = 0;
        dl->max = 1;
        dl->list = Z_Calloc(sizeof(vtxlist_t) * dl->max, PU_LEVEL, 0);
    }
}
