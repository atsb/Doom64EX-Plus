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
// DESCRIPTION:
//
//-----------------------------------------------------------------------------

#include "doomdef.h"
#include "doomstat.h"
#include "gl_main.h"
#include "gl_texture.h"
#include "r_lights.h"
#include "r_sky.h"
#include "r_drawlist.h"
#include "r_main.h"
#include "r_things.h"
#include "i_swap.h"
#include "i_system.h"
#include "dgl.h"
#include "con_cvar.h"
#include "m_fixed.h"

CVAR_EXTERNAL(r_texturecombiner);
CVAR_EXTERNAL(i_interpolateframes);
CVAR_EXTERNAL(r_fog);
CVAR_EXTERNAL(r_rendersprites);
CVAR_EXTERNAL(st_flashoverlay);

int game_world_shader_scope = 0;

extern void I_SectorCombiner_SetFog(int en, float r, float g, float b, float fac);
extern void I_SectorCombiner_SetFogParams(int mode, float start, float end, float density);

//
// ProcessWalls
//

static boolean ProcessWalls(vtxlist_t* vl, int* drawcount) {
	seg_t* seg = (seg_t*)vl->data;
	sector_t* sec = seg->frontsector;

	bspColor[LIGHT_FLOOR] = R_GetSectorLight(0xff, sec->colors[LIGHT_FLOOR]);
	bspColor[LIGHT_CEILING] = R_GetSectorLight(0xff, sec->colors[LIGHT_CEILING]);
	bspColor[LIGHT_THING] = R_GetSectorLight(0xff, sec->colors[LIGHT_THING]);
	bspColor[LIGHT_UPRWALL] = R_GetSectorLight(0xff, sec->colors[LIGHT_UPRWALL]);
	bspColor[LIGHT_LWRWALL] = R_GetSectorLight(0xff, sec->colors[LIGHT_LWRWALL]);

	if (!vl->callback(seg, &drawVertex[*drawcount])) {
		return false;
	}

	dglTriangle(*drawcount + 0, *drawcount + 1, *drawcount + 2);
	dglTriangle(*drawcount + 3, *drawcount + 2, *drawcount + 1);

	*drawcount += 4;

	return true;
}

//
// ProcessFlats
//

static boolean ProcessFlats(vtxlist_t* vl, int* drawcount) {
	int j;
	fixed_t tx;
	fixed_t ty;
	leaf_t* leaf;
	subsector_t* ss;
	sector_t* sector;
	int count;

	ss = (subsector_t*)vl->data;
	leaf = &leafs[ss->leaf];
	sector = ss->sector;
	count = *drawcount;

	for (j = 0; j < ss->numleafs - 2; j++) {
		dglTriangle(count, count + 1 + j, count + 2 + j);
	}

	// need to keep texture coords small to avoid
	// floor 'wobble' due to rounding errors on some cards
	// make relative to first vertex, not (0,0)
	// which is arbitary anyway

	tx = (leaf->vertex->x >> 6) & ~(FRACUNIT - 1);
	ty = (leaf->vertex->y >> 6) & ~(FRACUNIT - 1);

	for (j = 0; j < ss->numleafs; j++) {
		int idx;
		vtx_t* v = &drawVertex[count];

		if (vl->flags & DLF_CEILING) {
			leaf = &leafs[(ss->leaf + (ss->numleafs - 1)) - j];
		}
		else {
			leaf = &leafs[ss->leaf + j];
		}

		v->x = F2D3D(leaf->vertex->x);
		v->y = F2D3D(leaf->vertex->y);

		if (vl->flags & DLF_CEILING) {
			if (i_interpolateframes.value) {
				v->z = F2D3D(sector->frame_z2[1]);
			}
			else {
				v->z = F2D3D(sector->ceilingheight);
			}
		}
		else {
			if (i_interpolateframes.value) {
				v->z = F2D3D(sector->frame_z1[1]);
			}
			else {
				v->z = F2D3D(sector->floorheight);
			}
		}

		v->tu = F2D3D((leaf->vertex->x >> 6) - tx);
		v->tv = -F2D3D((leaf->vertex->y >> 6) - ty);

		// set the mapping offsets for scrolling floors/ceilings
		if ((!(vl->flags & DLF_CEILING) && sector->flags & MS_SCROLLFLOOR) ||
			(vl->flags & DLF_CEILING && sector->flags & MS_SCROLLCEILING)) {
			v->tu += F2D3D(sector->xoffset >> 6);
			v->tv += F2D3D(sector->yoffset >> 6);
		}

		v->a = 0xff;

		if (vl->flags & DLF_CEILING) {
			idx = sector->colors[LIGHT_CEILING];
		}
		else {
			idx = sector->colors[LIGHT_FLOOR];
		}

		R_LightToVertex(v, idx, 1);

		//
		// water layer 1
		//
		if (vl->flags & DLF_WATER1) {
			v->tv -= F2D3D(scrollfrac >> 6);
			v->a = 0xA0;
		}

		//
		// water layer 2
		//
		if (vl->flags & DLF_WATER2) {
			v->tu += F2D3D(scrollfrac >> 6);
		}

		count++;
	}

	*drawcount = count;

	return true;
}

//
// ProcessSprites
//

static boolean ProcessSprites(vtxlist_t* vl, int* drawcount) {
	visspritelist_t* vis;
	mobj_t* mobj;

	vis = (visspritelist_t*)vl->data;
	mobj = vis->spr;

	if (!mobj) {
		return false;
	}

	if (!vl->callback(vis, &drawVertex[*drawcount])) {
		return false;
	}

	GL_SetState(GLSTATE_CULL, !(mobj->flags & MF_RENDERLASER));

	dglTriangle(*drawcount + 0, *drawcount + 1, *drawcount + 2);
	dglTriangle(*drawcount + 3, *drawcount + 2, *drawcount + 1);

	*drawcount += 4;

	return true;
}

//
// SetupFog
//
static void SetupFog(void) {
	if (r_fillmode.value <= 0) {
		I_SectorCombiner_SetFog(0, 0, 0, 0, 0);
		I_SectorCombiner_SetFogParams(0, 0, 0, 0);
		dglDisable(GL_FOG);
		return;
	}

	boolean has_fog = r_fog.value || (sky && sky->fogcolor != 0);

	if (!has_fog) {
		dglDisable(GL_FOG);
		I_SectorCombiner_SetFog(0, 0, 0, 0, 0);
		I_SectorCombiner_SetFogParams(0, 0, 0, 0);
		return;
	}

	rfloat color[4] = { 0,0,0,1 };
	rcolor fogcolor = 0;
	int fognear = sky ? sky->fognear : 985;
	int fogfactor = 1000 - fognear;

	if (fogfactor <= 0) fogfactor = 1;

	dglEnable(GL_FOG);

	if (sky) {
		fogcolor = sky->fogcolor;
	}

	color[0] = ((fogcolor & 0xFF)) / 255.0f;
	color[1] = ((fogcolor >> 8) & 0xFF) / 255.0f; 
	color[2] = ((fogcolor >> 16) & 0xFF) / 255.0f;
	color[3] = 1.0f;

	float position = ((float)fogfactor) / 1000.0f;
	if (position <= 0.0f) position = 0.00001f;

	float start = 5.0f / position;
	float end = 30.0f / position;

	dglFogi(GL_FOG_MODE, GL_LINEAR);
	dglFogf(GL_FOG_START, start);
	dglFogf(GL_FOG_END, end);
	I_SectorCombiner_SetFogParams(1, start, end, 0.0f);

	dglFogfv(GL_FOG_COLOR, color);
	I_SectorCombiner_SetFog(1, color[0], color[1], color[2], (float)fogfactor / 1000.0f);
}

//
// R_SetViewMatrix
//

void R_SetViewMatrix(void) {
	dglMatrixMode(GL_PROJECTION);
	dglLoadIdentity();
	dglViewFrustum(video_width, video_height, r_fov.value, 0.1f);
	dglMatrixMode(GL_MODELVIEW);
	dglLoadIdentity();
	dglRotatef(-TRUEANGLES(viewpitch), 1.0f, 0.0f, 0.0f);
	dglRotatef(-TRUEANGLES(viewangle) + 90.0f, 0.0f, 0.0f, 1.0f);
	dglTranslatef(-fviewx, -fviewy, -fviewz);
}

//
// R_RenderWorld
//

void R_RenderWorld(void) {
	game_world_shader_scope = 1;
	I_ShaderBind();
	SetupFog();

	if (sky && (sky->flags & SKF_VOID)) {
		byte* vb = (byte*)&sky->skycolor[2];
		dglClearColor(vb[0] / 255.0f, vb[1] / 255.0f, vb[2] / 255.0f, 1.0f);
		dglClear(GL_COLOR_BUFFER_BIT);
	}

	dglEnable(GL_DEPTH_TEST);
	DL_BeginDrawList(r_fillmode.value >= 1, r_texturecombiner.value >= 1);

	// setup texture environment for effects
	if (r_texturecombiner.value) {
		if (!nolights) {
			GL_UpdateEnvTexture(WHITE);
			GL_SetTextureUnit(1, true);
			dglTexCombModulate(GL_PREVIOUS, GL_PRIMARY_COLOR);
		}
		if (st_flashoverlay.value <= 0) {
			GL_SetTextureUnit(2, true);
			dglTexCombColor(GL_PREVIOUS, flashcolor, GL_ADD);
		}
		dglTexCombReplaceAlpha(GL_TEXTURE0_ARB);
		GL_SetTextureUnit(0, true);
	}

	dglEnable(GL_ALPHA_TEST);
	dglAlphaFunc(GL_GREATER, 0.5f);
	GL_SetState(GLSTATE_BLEND, 0);
	DL_ProcessDrawList(DLT_FLAT, ProcessFlats);
	DL_ProcessDrawList(DLT_WALL, ProcessWalls);

	dglDisable(GL_ALPHA_TEST);
	dglDepthMask(GL_FALSE);
	GL_SetState(GLSTATE_BLEND, 1);
	dglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	DL_ProcessDrawList(DLT_FLAT, ProcessFlats);
	DL_ProcessDrawList(DLT_WALL, ProcessWalls);

	if (r_rendersprites.value) {
		R_SetupSprites();
		dglBlendFunc(GL_SRC_ALPHA, GL_ONE);
		GL_SetState(GLSTATE_BLEND, 1);
		DL_ProcessDrawList(DLT_SPRITE, ProcessSprites);
	}

	// Restore states
	dglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	GL_SetState(GLSTATE_BLEND, 0);
	dglEnable(GL_ALPHA_TEST);
	dglDepthMask(GL_TRUE);
	dglDisable(GL_FOG);
	dglDisable(GL_DEPTH_TEST);
	GL_SetOrthoScale(1.0f);
	GL_SetDefaultCombiner();
	I_ShaderUnBind();
}
