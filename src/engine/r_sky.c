// Emacs style mode select   -*- C -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 1993-1997 Id Software, Inc.
// Copyright(C) 1997 Midway Home Entertainment, Inc
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
// DESCRIPTION: Sky rendering code
//
//-----------------------------------------------------------------------------

#include <stdlib.h>
#include <math.h>

#include "doomstat.h"
#include "r_lights.h"
#include "r_sky.h"
#include "w_wad.h"
#include "m_random.h"
#include "sounds.h"
#include "s_sound.h"
#include "p_local.h"
#include "z_zone.h"
#define close _close
#include "i_png.h"
#undef _close
#include "gl_texture.h"
#include "gl_draw.h"
#include "r_drawlist.h"
#include "con_cvar.h"
#include "r_main.h"
#include "dgl.h"
#include "i_sectorcombiner.h"

extern vtx_t drawVertex[MAXDLDRAWCOUNT];
extern void I_ShaderSetTextureSize(int w, int h);
extern void I_ShaderSetUseTexture(int on);

skydef_t* sky;
int         skypicnum = -1;
int         skybackdropnum = -1;
int         skyflatnum = -1;
int         thunderCounter = 0;
int         lightningCounter = 0;
int         thundertic = 1;
bool        skyfadeback = false;
byte* fireBuffer;
dPalette_t  firePal16[256];
int         fireLumpGfxId;

static word CloudOffsetY = 0;
static word CloudOffsetX = 0;
static float sky_cloudpan1 = 0;
static float sky_cloudpan2 = 0;

#define FIRESKY_WIDTH   64
#define FIRESKY_HEIGHT  64

CVAR_EXTERNAL(r_texturecombiner);
CVAR_EXTERNAL(r_fov);
CVAR_EXTERNAL(r_skyFilter);
CVAR(r_skybox, 1);
#define SKYVIEWPOS(angle, amount, x) x = -(angle / (float)ANG90 * amount); while(x < 1.0f) x += 1.0f

// atsb: crappy hack
static rcolor PostProcessSkyColor(rcolor original_color, boolean is_cloud) {
    int r = (original_color >> 0) & 0xFF;
    int g = (original_color >> 8) & 0xFF;
    int b = (original_color >> 16) & 0xFF;
    int a = (original_color >> 24) & 0xFF;

    r = (r * 130) / 100;
    b = (b * 180) / 100;
    g = (g * 140) / 100;

    if (r < 0) r = 0;
    if (r > 255) r = 255;
    if (g < 0) g = 0;
    if (g > 255) g = 255;
    if (b < 0) b = 0;
    if (b > 255) b = 255;

    return r | (g << 8) | (b << 16) | (a << 24);
}

// atsb: a small little function to not have to copy and paste so much
static inline void R_NeutralizeShaders(void) {
    I_ShaderUnBind();
    I_SectorCombiner_Unbind();
    I_ShaderSetUseTexture(1);
    I_ShaderSetTextureSize(0, 0);
}

static GLuint gCloudAlphaTex = 0;
static int gCloudAlphaForLump = -1;

static void R_BuildCloudAlphaTexture(int lumpNum)
{
    if (gCloudAlphaForLump == lumpNum && gCloudAlphaTex)
        return;

    GLint w = 0, h = 0;
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h);
    if (w <= 0 || h <= 0)
        return;

    size_t n = (size_t)w * (size_t)h * 4;
    unsigned char* src = (unsigned char*)Z_Malloc(n, PU_STATIC, 0);
    unsigned char* dst = (unsigned char*)Z_Malloc(n, PU_STATIC, 0);
    if (!src || !dst) { if (src) Z_Free(src); if (dst) Z_Free(dst); return; }

    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, src);

    for (int i = 0; i < w * h; ++i) {
        unsigned char r = src[i * 4 + 0];
        unsigned char g = src[i * 4 + 1];
        unsigned char b = src[i * 4 + 2];
        // luminance -> alpha
        unsigned int a = (unsigned int)(0.2126f * r + 0.7152f * g + 0.0722f * b + 0.5f);
        dst[i * 4 + 0] = r;
        dst[i * 4 + 1] = g;
        dst[i * 4 + 2] = b;
        dst[i * 4 + 3] = (unsigned char)a;
    }

    if (!gCloudAlphaTex) {
        dglGenTextures(1, &gCloudAlphaTex);
    }
    dglBindTexture(GL_TEXTURE_2D, gCloudAlphaTex);
    dglTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, dst);
    dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (int)r_skyFilter.value == 0 ? GL_LINEAR : GL_NEAREST);
    dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (int)r_skyFilter.value == 0 ? GL_LINEAR : GL_NEAREST);
    dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    Z_Free(src);
    Z_Free(dst);

    gCloudAlphaForLump = lumpNum;
}

//
// R_CloudThunder
// Loosely based on subroutine at 0x80026418
//

static void R_CloudThunder(void) {
    if (!(gametic & ((thunderCounter & 1) ? 1 : 3))) {
        return;
    }

    if ((thunderCounter - thundertic) > 0) {
        thunderCounter = (thunderCounter - thundertic);
        return;
    }

    if (lightningCounter == 0) {
        S_StartSound(NULL, sfx_thndrlow + (M_Random() & 1));
        thundertic = (1 + (M_Random() & 1));
    }

    if (!(lightningCounter < 6)) {  // Reset loop after 6 lightning flickers
        int rand = (M_Random() & 7);
        thunderCounter = (((rand << 4) - rand) << 2) + 60;
        lightningCounter = 0;
        return;
    }

    if ((lightningCounter & 1) == 0) {
        sky->skycolor[0] += 0x001111;
        sky->skycolor[1] += 0x001111;
    }
    else {
        sky->skycolor[0] -= 0x001111;
        sky->skycolor[1] -= 0x001111;
    }

    thunderCounter = (M_Random() & 7) + 1;    // Do short delay loops for lightning flickers
    lightningCounter++;
}

//
// R_CloudTicker
//

static void R_CloudTicker(void) {
    CloudOffsetX -= (dcos(viewangle) >> 10);
    CloudOffsetY += (dsin(viewangle) >> 9);

    if (r_skybox.value) {
        sky_cloudpan1 += 0.00225f;
        sky_cloudpan2 += 0.00075f;
    }

    if (sky->flags & SKF_THUNDER) {
        R_CloudThunder();
    }
}

//
// R_TitleSkyTicker
//

static void R_TitleSkyTicker(void) {
    if (skyfadeback == true) {
        logoAlpha += 8;
        if (logoAlpha > 0xff) {
            logoAlpha = 0xff;
            skyfadeback = false;
        }
    }
}

//
// R_DrawSkyDome
//

// atsb: largely rewritten

/* how this works now is that we draw a ring around the 'border' of the void
we set it at the pitch angle and yaw of the camera and lock it steady
we then push/pop from the depth buffer so that there is no fighting against geometry and skies
after we finish writing, we turn the depth test off to restore the standard rendering

skies are also 'only' GL_LINEAR.  GL_NEAREST skies do NOT look good
*/
static void R_DrawSkyDome(int tiles, float rows, int height,
    int radius, float offset, float topoffs,
    rcolor c1, rcolor c2) {
    fixed_t x, y, z;
    fixed_t lx, ly;
    fixed_t rx, ry;
    int i;
    angle_t an;
    float tu1, tu2;
    int r;
    vtx_t* vtx;
    int count;

    lx = ly = count = 0;

#define NUM_SKY_DOME_FACES  32

    GL_SetOrthoScale(1.0f);

    //
    // setup view projection
    //
    dglMatrixMode(GL_PROJECTION);
    dglPushMatrix();
    dglLoadIdentity();
    dglViewFrustum(video_width, video_height, r_fov.value, 0.1f);
    dglMatrixMode(GL_MODELVIEW);
    dglLoadIdentity();
    dglPushMatrix();
    dglRotatef(-TRUEANGLES(viewpitch), 1.0f, 0.0f, 0.0f);

    dglDisable(GL_DEPTH_TEST);
    dglDepthMask(GL_FALSE);
    dglRotatef(-TRUEANGLES(viewangle) + 90.0f, 0.0f, 0.0f, 1.0f);
    dglTranslated(0.0f, 0.0f, -offset);

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_PRIMARY_COLOR);

    // atsb: prevents z-buffer fighting
    dglDisable(GL_DEPTH_TEST);
    dglDepthMask(GL_FALSE);

    //
    // front faces are drawn here, so cull the back faces
    //
    dglCullFace(GL_BACK);
    GL_SetState(GLSTATE_BLEND, 1);
    dglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    dglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    GLint old2DMin, old2DMag;
    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, &old2DMin);
    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, &old2DMag);
    GLint oldWrapS, oldWrapT;
    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, &oldWrapS);
    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, &oldWrapT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (int)r_skyFilter.value == 0 ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (int)r_skyFilter.value == 0 ? GL_LINEAR : GL_NEAREST);
#ifdef GL_TEXTURE_CUBE_MAP
    GLint oldCubeMin, oldCubeMag;
    glGetTexParameteriv(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, &oldCubeMin);
    glGetTexParameteriv(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, &oldCubeMag);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, (int)r_skyFilter.value == 0 ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, (int)r_skyFilter.value == 0 ? GL_LINEAR : GL_NEAREST);
#endif

    // void radius of the sky
    r = radius;

    //
    // set pointer for the main vertex list
    //
    dglSetVertex(drawVertex);
    vtx = drawVertex;

#define SKYDOME_VERTEX() vtx->x = F2D3D(x); vtx->y = F2D3D(y); vtx->z = F2D3D(z)
#define SKYDOME_UV(u, v) vtx->tu = u; vtx->tv = v
#define SKYDOME_LEFT(v, h)                      \
    x = lx;                                     \
    y = ly;                                     \
    z = INT2F(h);                               \
    SKYDOME_UV(-tu1, v);                        \
    SKYDOME_VERTEX();                           \
    vtx++

#define SKYDOME_RIGHT(v, h)                     \
    x = rx;                                     \
    y = ry;                                     \
    z = INT2F(h);                               \
    SKYDOME_UV(-(tu2 * (i + 1)), v);            \
    SKYDOME_VERTEX();                           \
    vtx++

    tu1 = 0;
    tu2 = (float)tiles / (float)NUM_SKY_DOME_FACES;
    an = (ANGLE_MAX / NUM_SKY_DOME_FACES);

    //
    // setup vertex data
    //
    for (i = 0; i < NUM_SKY_DOME_FACES; i++) {
        angle_t a0 = an * i;
        angle_t a1 = an * (i + 1);

        lx = FixedMul(INT2F(r), dcos(a0));
        ly = FixedMul(INT2F(r), dsin(a0));
        rx = FixedMul(INT2F(r), dcos(a1));
        ry = FixedMul(INT2F(r), dsin(a1));

        dglSetVertexColor(&vtx[0], c2, 1);
        dglSetVertexColor(&vtx[1], c1, 1);
        dglSetVertexColor(&vtx[2], c1, 1);
        dglSetVertexColor(&vtx[3], c2, 1);

        SKYDOME_LEFT(1.0f, -height);
        SKYDOME_LEFT(0.0f, height);
        SKYDOME_RIGHT(0.0f, height);
        SKYDOME_RIGHT(1.0f, -height);

        dglTriangle(0 + count, 1 + count, 2 + count);
        dglTriangle(3 + count, 0 + count, 2 + count);
        count += 4;

        tu1 += tu2;
    }

    //
    // draw sky dome
    //
    dglDrawGeometry(count, drawVertex);

    // atsb: the below restore the renderer filter and also pops the depth test back
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, old2DMin);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, old2DMag);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, oldWrapS);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, oldWrapT);
#ifdef GL_TEXTURE_CUBE_MAP
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, oldCubeMin);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, oldCubeMag);
#endif

    dglDepthMask(GL_TRUE);
    dglEnable(GL_DEPTH_TEST);

    dglPopMatrix();
    dglMatrixMode(GL_PROJECTION);
    dglPopMatrix();
    dglMatrixMode(GL_MODELVIEW);

    dglCullFace(GL_FRONT);
    GL_SetState(GLSTATE_BLEND, 0);

#undef SKYDOME_RIGHT
#undef SKYDOME_LEFT
#undef SKYDOME_UV
#undef SKYDOME_VERTEX
#undef NUM_SKY_DOME_FACES
}

//
// R_DrawSimpleSky
//

static void R_DrawSimpleSky(int lump, int offset) {
    float pos1;
    float width;
    int height;
    int lumpheight;
    int gfxLmp;
    float row;

    float skyScaleX = 1.3f;
    float skyScaleY = 1.0f;

    I_ShaderUnBind();

    gfxLmp = GL_BindGfxTexture(lumpinfo[lump].name, true);

    dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (int)r_skyFilter.value == 0 ? GL_LINEAR : GL_NEAREST);
    dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (int)r_skyFilter.value == 0 ? GL_LINEAR : GL_NEAREST);

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_PRIMARY_COLOR);
    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_TEXTURE);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.2f);

    height = gfxheight[gfxLmp];
    lumpheight = gfxorigheight[gfxLmp];

    dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    SKYVIEWPOS(viewangle, 1, pos1);

    width = ((float)SCREENWIDTH / (float)gfxwidth[gfxLmp]) * skyScaleX;
    row = ((float)lumpheight / (float)height) * skyScaleY;

    GL_SetState(GLSTATE_BLEND, 1);
    dglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    GL_SetupAndDraw2DQuad(0, (float)offset - lumpheight, SCREENWIDTH, lumpheight,
        pos1, width + pos1, 0.006f, row, WHITE, 1);

    GL_SetState(GLSTATE_BLEND, 0);

    glDisable(GL_ALPHA_TEST);
    dglPopMatrix();
    GL_ResetViewport();
    I_ShaderBind();
}

//
// R_DrawVoidSky
//

static void R_DrawVoidSky(void) {
    GL_SetOrtho(1);

    dglDisable(GL_TEXTURE_2D);
    dglColor4ubv((byte*)&sky->skycolor[2]);
    dglRecti(SCREENWIDTH, SCREENHEIGHT, 0, 0);
    dglEnable(GL_TEXTURE_2D);

    GL_ResetViewport();
}

//
// R_DrawTitleSky
//

static void R_DrawTitleSky(void) {
    R_DrawSimpleSky(skypicnum, 240);
    Draw_GfxImage(63, 25, sky->backdrop, D_RGBA(255, 255, 255, logoAlpha), false);
}

//
// R_DrawClouds
//

static void R_DrawClouds(void) {

    R_NeutralizeShaders();
    rfloat pos = 0.0f;
    vtx_t v[4];

    I_ShaderUnBind();
    GL_SetTextureUnit(0, true);
    R_NeutralizeShaders();
    GL_BindGfxTexture(lumpinfo[skypicnum].name, true);
    dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (int)r_skyFilter.value == 0 ? GL_LINEAR : GL_NEAREST);
    dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (int)r_skyFilter.value == 0 ? GL_LINEAR : GL_NEAREST);

    R_BuildCloudAlphaTexture(skypicnum);
    if (gCloudAlphaTex)
        dglBindTexture(GL_TEXTURE_2D, gCloudAlphaTex);

    pos = (TRUEANGLES(viewangle) / 360.0f) * 2.0f;

    dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    GLboolean wasAlphaTest = glIsEnabled(GL_ALPHA_TEST);
    GLboolean wasBlend = glIsEnabled(GL_BLEND);
    GLint previousBlendSrc = 0, previousBlendDst = 0;
#ifdef GL_BLEND_SRC
    glGetIntegerv(GL_BLEND_SRC, &previousBlendSrc);
    glGetIntegerv(GL_BLEND_DST, &previousBlendDst);
#else
    previousBlendSrc = GL_SRC_ALPHA;
    previousBlendDst = GL_ONE_MINUS_SRC_ALPHA;
#endif

    dglDisable(GL_ALPHA_TEST);
    dglEnable(GL_BLEND);
    dglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    dglSetVertex(v);

    I_ShaderUnBind();
    GL_Set2DQuad(v, 0, 0, SCREENWIDTH, SCREENHEIGHT, 0, 0, 0, 0, 0);
    dglSetVertexColor(&v[0], PostProcessSkyColor(sky->skycolor[1], true), 4);
    dglSetVertexColor(&v[2], PostProcessSkyColor(sky->skycolor[0], false), 2); // bottom
    dglDisable(GL_TEXTURE_2D);
    GL_Draw2DQuad(v, true);
    dglEnable(GL_TEXTURE_2D);

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_PRIMARY_COLOR);
    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_TEXTURE);

    dglSetVertexColor(&v[0], sky->skycolor[2], 4);

    I_ShaderUnBind();
    GL_Set2DQuad(v, 0, 0, SCREENWIDTH, SCREENHEIGHT, 0, 0, 0, 0, 0);
    dglSetVertexColor(&v[0], PostProcessSkyColor(sky->skycolor[1], true), 4);
    dglSetVertexColor(&v[2], PostProcessSkyColor(sky->skycolor[0], false), 2);

    dglDisable(GL_TEXTURE_2D);

    GL_Draw2DQuad(v, true);

    dglEnable(GL_TEXTURE_2D);

    GL_SetTextureUnit(1, true);
    R_NeutralizeShaders();
    GL_SetTextureMode(GL_ADD);
    GL_UpdateEnvTexture(sky->skycolor[1]);
    GL_SetTextureUnit(0, true);
    R_NeutralizeShaders();

    dglSetVertexColor(&v[0], sky->skycolor[2], 4);
    v[0].a = v[1].a = v[2].a = v[3].a = 0x60;

    v[3].x = v[1].x = 1.1025f;
    v[0].x = v[2].x = -1.1025f;
    v[2].y = v[3].y = 0;
    v[0].y = v[1].y = 0.4315f;
    v[0].z = v[1].z = 0;
    v[2].z = v[3].z = -1.0f;
    v[0].tu = v[2].tu = F2D3D(CloudOffsetX) - pos;
    v[1].tu = v[3].tu = (F2D3D(CloudOffsetX) + 1.5f) - pos;
    v[0].tv = v[1].tv = F2D3D(CloudOffsetY);
    v[2].tv = v[3].tv = F2D3D(CloudOffsetY) + 2.0f;

    GL_SetOrthoScale(1.0f);

    dglMatrixMode(GL_PROJECTION);
    dglLoadIdentity();
    dglViewFrustum(SCREENWIDTH, SCREENHEIGHT, 45.0f, 0.1f);
    dglMatrixMode(GL_MODELVIEW);
    dglEnable(GL_BLEND);
    dglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Draw the 3D overlay without touching depth, then restore.
    GLboolean hadDepth = glIsEnabled(GL_DEPTH_TEST);
    GLboolean previousDepth = GL_TRUE;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &previousDepth);
    dglDisable(GL_DEPTH_TEST);
    dglDepthMask(GL_FALSE);

    dglPushMatrix();
    dglTranslated(0.0f, 0.0f, -1.0f);
    dglTriangle(0, 1, 2);
    dglTriangle(3, 2, 1);
    dglDrawGeometry(4, v);
    dglPopMatrix();

    dglDepthMask(previousDepth);
    if (hadDepth)
        dglEnable(GL_DEPTH_TEST);
    else
        dglDisable(GL_DEPTH_TEST);

    glBlendFunc(previousBlendSrc, previousBlendDst);
    if (wasAlphaTest)
        dglEnable(GL_ALPHA_TEST);
    else
        dglDisable(GL_ALPHA_TEST);
    if (wasBlend)
        dglEnable(GL_BLEND);
    else
        dglDisable(GL_BLEND);

    GL_SetDefaultCombiner();
    I_ShaderBind();
}

//
// R_SpreadFire
//

static void R_SpreadFire(byte* src1, byte* src2, int pixel, int counter, int* rand) {
    int randIdx = 0;
    byte* tmpSrc;

    if (pixel != 0) {
        randIdx = rndtable[*rand];
        *rand = ((*rand + 2) & 0xff);

        tmpSrc = (src1 + (((counter - (randIdx & 3)) + 1) & (FIRESKY_WIDTH - 1)));
        *(byte*)(tmpSrc - FIRESKY_WIDTH) = pixel - ((randIdx & 1));
    }
    else {
        *(byte*)(src2 - FIRESKY_WIDTH) = 0;
    }
}

//
// R_Fire
//

static void R_Fire(byte* buffer) {
    int counter = 0;
    int rand = 0;
    int step = 0;
    int pixel = 0;
    byte* src;
    byte* srcoffset;

    rand = (M_Random() & 0xff);
    src = buffer;
    counter = 0;
    src += FIRESKY_WIDTH;

    do {  // width
        srcoffset = (src + counter);
        pixel = *(byte*)srcoffset;

        step = 2;

        R_SpreadFire(src, srcoffset, pixel, counter, &rand);

        src += FIRESKY_WIDTH;
        srcoffset += FIRESKY_WIDTH;

        do {  // height
            pixel = *(byte*)srcoffset;
            step += 2;

            R_SpreadFire(src, srcoffset, pixel, counter, &rand);

            pixel = *(byte*)(srcoffset + FIRESKY_WIDTH);
            src += FIRESKY_WIDTH;
            srcoffset += FIRESKY_WIDTH;

            R_SpreadFire(src, srcoffset, pixel, counter, &rand);

            src += FIRESKY_WIDTH;
            srcoffset += FIRESKY_WIDTH;

        } while (step < FIRESKY_HEIGHT);

        counter++;
        src -= ((FIRESKY_WIDTH * FIRESKY_HEIGHT) - FIRESKY_WIDTH);

    } while (counter < FIRESKY_WIDTH);
}

//
// R_InitFire
//

static rcolor firetexture[FIRESKY_WIDTH * FIRESKY_HEIGHT];

void R_InitFire(void) {
    int i;

    int fireLump = W_GetNumForName("FIRE") - g_start;
    fireLumpGfxId = GL_GetGfxIdForLump(fireLump);
    dmemset(&firePal16, 0, sizeof(dPalette_t) * 256);
    for (i = 0; i < 16; i++) {
        firePal16[i].r = 16 * i;
        firePal16[i].g = 16 * i;
        firePal16[i].b = 16 * i;
        firePal16[i].a = 0xff;
    }

    fireBuffer = I_PNGReadData(g_start + fireLump,
        true, true, false, 0, 0, 0, 0);

    for (i = 0; i < 4096; i++) {
        fireBuffer[i] >>= 4;
    }
}

//
// R_FireTicker
//

static void R_FireTicker(void) {
    if (leveltime & 1) {
        R_Fire(fireBuffer);
    }
}

//
// R_DrawFire
//

static void R_DrawFire(void) {
    R_NeutralizeShaders();
    I_ShaderUnBind();
    GL_SetTextureUnit(0, true);
    float pos1;
    vtx_t v[4];
    dtexture t = gfxptr[fireLumpGfxId];
    int i;

    //
    // copy fire pixel data to texture data array
    //
    dmemset(firetexture, 0, sizeof(int) * FIRESKY_WIDTH * FIRESKY_HEIGHT);
    for (i = 0; i < FIRESKY_WIDTH * FIRESKY_HEIGHT; i++) {
        byte rgb[3];

        rgb[0] = firePal16[fireBuffer[i]].r;
        rgb[1] = firePal16[fireBuffer[i]].g;
        rgb[2] = firePal16[fireBuffer[i]].b;

        firetexture[i] = D_RGBA(rgb[2], rgb[1], rgb[0], 0xff);
    }

    if (!t) {
        dglGenTextures(1, &gfxptr[fireLumpGfxId]);
    }

    dglBindTexture(GL_TEXTURE_2D, gfxptr[fireLumpGfxId]);
    GL_CheckFillMode();
    dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (int)r_skyFilter.value == 0 ? GL_LINEAR : GL_NEAREST);
    dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (int)r_skyFilter.value == 0 ? GL_LINEAR : GL_NEAREST);

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_PRIMARY_COLOR);
    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_TEXTURE);
    if (devparm) {
        glBindCalls++;
    }


    if (!t) {
        //
        // copy data if it didn't exist before
        //
        dglTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGBA8,
            FIRESKY_WIDTH,
            FIRESKY_HEIGHT,
            0,
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            firetexture
        );
    }
    else {
        //
        // update texture data
        //
        dglTexSubImage2D(
            GL_TEXTURE_2D,
            0,
            0,
            0,
            FIRESKY_WIDTH,
            FIRESKY_HEIGHT,
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            firetexture
        );
    }

    if (r_skybox.value <= 0) {
        SKYVIEWPOS(viewangle, 4, pos1);

        //
        // adjust UV by 0.0035f units due to the fire sky showing a
        // strip of color going along the top portion of the texture
        //
        GL_Set2DQuad(v, 0, 0, SCREENWIDTH, 120,
            pos1, 5.0f + pos1, 0.0035f, 1.0f, 0);

        dglSetVertexColor(&v[0], PostProcessSkyColor(sky->skycolor[0], false), 2);
        dglSetVertexColor(&v[2], PostProcessSkyColor(sky->skycolor[1], false), 2);

        GL_Draw2DQuad(v, 1);
    }
    else {
        R_DrawSkyDome(16, 1, 1024, 4096, -896, 0.0075f,
            sky->skycolor[0], sky->skycolor[1]);
    }
    GL_SetDefaultCombiner();
    I_ShaderBind();
}

//
// R_DrawSky
//

void R_DrawSky(void) {

    if (!sky) {
        return;
    }

    if (sky->flags & SKF_VOID) {
        R_DrawVoidSky();
    }
    else if (skypicnum >= 0) {
        if (sky->flags & SKF_CLOUD) {
            if (r_skybox.value <= 0) {
                R_DrawSimpleSky(skypicnum, 128);
                R_DrawClouds();
            }
        }
        else {
            if (r_skybox.value <= 0) {
                R_DrawSimpleSky(skypicnum, 128);
            }
            else {
                GL_SetTextureUnit(0, true);
                R_NeutralizeShaders();
                GL_BindGfxTexture(lumpinfo[skypicnum].name, true);
                dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (int)r_skyFilter.value == 0 ? GL_LINEAR : GL_NEAREST);
                dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (int)r_skyFilter.value == 0 ? GL_LINEAR : GL_NEAREST);

                //
                // drawer will assume that the texture's
                // dimensions is already in powers of 2
                //
                R_DrawSkyDome(4, 2, 512, 1024,
                    0, 0, WHITE, WHITE);
            }
        }
    }

    if (sky->flags & SKF_FIRE) {
        R_DrawFire();
    }

    if (skybackdropnum >= 0) {
        if (sky->flags & SKF_FADEBACK) {
            R_DrawTitleSky();
        }
        else if (sky->flags & SKF_BACKGROUND) {
            if (r_skybox.value <= 0) {
                R_DrawSimpleSky(skybackdropnum, 170);
            }
            else {
                float h;
                float origh;
                int l;
                int domeheight;
                float offset;
                float base;

                GL_SetTextureUnit(0, true);
                R_NeutralizeShaders();
                l = GL_BindGfxTexture(lumpinfo[skybackdropnum].name, true);
                dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (int)r_skyFilter.value == 0 ? GL_LINEAR : GL_NEAREST);
                dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (int)r_skyFilter.value == 0 ? GL_LINEAR : GL_NEAREST);

                //
                // handle the case for non-powers of 2 texture
                // dimensions. height and offset is adjusted
                // accordingly
                //
                origh = (float)gfxorigheight[l];
                h = (float)gfxheight[l];
                base = 160.0f - ((128 - origh) / 2.0f);
                domeheight = (int)(base / (origh / h));
                offset = (float)domeheight - base - 16.0f;

                R_DrawSkyDome(5, 1, domeheight, 768,
                    offset, 0.005f, WHITE, WHITE);
            }
        }
    }
}

//
// R_SkyTicker
//

void R_SkyTicker(void) {
    if (menuactive) {
        return;
    }

    if (!sky) {
        return;
    }

    if (sky->flags & SKF_CLOUD) {
        R_CloudTicker();
    }

    if (sky->flags & SKF_FIRE) {
        R_FireTicker();
    }

    if (sky->flags & SKF_FADEBACK) {
        R_TitleSkyTicker();
    }
}
