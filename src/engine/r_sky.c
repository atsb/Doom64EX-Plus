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

/* some emacs love */

/****************************************************************



             /-------------\
            /		        \
            /		         \
            /			      \
            |	XXXX	XXXX  |
            |   XXXX	XXXX  |
            |	XXX	    XXX	  |
             \		X	    /
            --\    XXX     /--
            | |    XXX    | |
            | |	          | |
            | I I I I I I I |
            |  I I I I I I	|
             \	           /
              --	     --
                \-------/
            XXX			   XXX
           XXXXX		  XXXXX
           XXXXXXXXX	  XXXXXXXXXX
              XXXXX	  XXXXX
                 XXXXXXX
              XXXXX	  XXXXX
           XXXXXXXXX	  XXXXXXXXXX
           XXXXX		  XXXXX
            XXX			   XXX

              **************
              *  BEWARE!!  *
              **************

            All ye who enter here:
            Most of the code in this module
            is twisted beyond belief!

               Tread carefully.

            If you think you understand it,
                  You Don't,
                So Look Again.

 ****************************************************************/

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
boolean        skyfadeback = false;
byte* fireBuffer;
dPalette_t  firePal16[256];
int         fireLump = -1;

static word CloudOffsetY = 0;
static word CloudOffsetX = 0;
static float sky_cloudpan1 = 0;
static float sky_cloudpan2 = 0;

#define FIRESKY_WIDTH   64
#define FIRESKY_HEIGHT  64

CVAR_EXTERNAL(r_texturecombiner);
CVAR_EXTERNAL(r_fov);
CVAR_EXTERNAL(r_skyFilter);
int r_skybox = 1;

GLint oldMin = 0, oldMax = 0;

#define SKYVIEWPOS(angle, amount, x) x = -(angle / (float)ANG90 * amount); while(x < 1.0f) x += 1.0f

// atsb: disgusting hacks!
// BACKPIC_HEIGHT_MODE controls vertical sizing behavior:
//   0 = Legacy
//   1 = Use caller
//   2 = Multiply caller drawHeight
//   3 = Use BACKPIC_FIXED_HEIGHT
//   4 = Clamp caller drawHeight
#ifndef BACKPIC_HEIGHT_MODE
#define BACKPIC_HEIGHT_MODE 3 // DO NOT CHANGE!
#endif

#ifndef BACKPIC_HEIGHT_SCALE
#define BACKPIC_HEIGHT_SCALE 1.0f
#endif

#ifndef BACKPIC_FIXED_HEIGHT
#define BACKPIC_FIXED_HEIGHT 100
#endif

#ifndef BACKPIC_MIN_HEIGHT
#define BACKPIC_MIN_HEIGHT 0
#endif
#ifndef BACKPIC_MAX_HEIGHT
#define BACKPIC_MAX_HEIGHT 0
#endif

// Extra pixel offset
#ifndef BACKPIC_EXTRA_Y_OFFSET
#define BACKPIC_EXTRA_Y_OFFSET 0.0f
#endif

// Horizontal
#ifndef BACKPIC_U_CYCLES
#define BACKPIC_U_CYCLES 1.0f
#endif

// Vertical
#ifndef BACKPIC_WRAP_T_CLAMP
#define BACKPIC_WRAP_T_CLAMP 1
#endif

// Alpha cutoff
#ifndef BACKPIC_ALPHA_THRESHOLD
#define BACKPIC_ALPHA_THRESHOLD 0.5f
#endif

// Vertical alignment:
//   0 = bottom-aligned
//   1 = top-aligned
//   2 = centered
#ifndef BACKPIC_ALIGNMENT
#define BACKPIC_ALIGNMENT 0
#endif

//   0 = use caller
//   1 = screen
//   2 = classic
#ifndef BACKPIC_POSITION_MODE
#define BACKPIC_POSITION_MODE 2
#endif

#ifndef BACKPIC_BASE_BIAS
#define BACKPIC_BASE_BIAS 16.0f
#endif

#ifndef BACKPIC_LARGE_FIXED_HEIGHT
#define BACKPIC_LARGE_FIXED_HEIGHT 250
#endif

#ifndef BACKPIC_MEDIUM_FIXED_HEIGHT
#define BACKPIC_MEDIUM_FIXED_HEIGHT 180
#endif

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

    if (r_skybox) {
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

    GLint old2DMin, old2DMag;
    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, &old2DMin);
    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, &old2DMag);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (int)r_skyFilter.value == 0 ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (int)r_skyFilter.value == 0 ? GL_LINEAR : GL_NEAREST);

    GLint oldWrapS, oldWrapT;
    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, &oldWrapS);
    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, &oldWrapT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

#ifdef GL_TEXTURE_CUBE_MAP
    GLint oldCubeMin, oldCubeMag;
    glGetTexParameteriv(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, &oldCubeMin);
    glGetTexParameteriv(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, &oldCubeMag);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, (int)r_skyFilter.value == 0 ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, (int)r_skyFilter.value == 0 ? GL_LINEAR : GL_NEAREST);
#endif

    r = radius;

    //
    // set pointer for the main vertex list
    //
    dglSetVertex(drawVertex);
    vtx = drawVertex;

#define SKYDOME_VERTEX() vtx->x = F2D3D(x); vtx->y = F2D3D(y); vtx->z = F2D3D(z)
#define SKYDOME_UV(u, v) vtx->tu = u; vtx->tv = (v > 0.5f ? (v - topoffs) : (v + topoffs))
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
// R_DrawSkyboxCloud
//

static void R_DrawSkyboxCloud(void) {
    vtx_t v[4];

    I_ShaderUnBind();

    I_ShaderSetUseTexture(1);
    I_ShaderSetTextureSize(0, 0);

#define SKYBOX_SETALPHA(c, x)           \
    c ^= (((c >> 24) & 0xff) << 24);    \
    c |= (x << 24)

    //
    // hack to force ortho scale back to 1
    //
    GL_SetOrthoScale(1.0f);

    //
    // setup view projection
    //
    dglMatrixMode(GL_PROJECTION);
    dglLoadIdentity();
    dglViewFrustum(video_width, video_height, r_fov.value, 0.1f);
    dglMatrixMode(GL_MODELVIEW);
    dglLoadIdentity();
    dglPushMatrix();
    dglRotatef(-TRUEANGLES(viewpitch), 1.0f, 0.0f, 0.0f);

    dglDisable(GL_DEPTH_TEST);
    dglDepthMask(GL_FALSE);


    //
    // set vertex pointer
    //
    dglSetVertex(v);

    //
    // disable textures for horizon effect
    //
    dglDisable(GL_TEXTURE_2D);

    //
    // draw horizon ceiling
    //
    v[0].x = -MAX_COORD;
    v[0].y = -MAX_COORD;
    v[0].z = 512;
    v[1].x = MAX_COORD;
    v[1].y = -MAX_COORD;
    v[1].z = 512;
    v[2].x = MAX_COORD;
    v[2].y = MAX_COORD;
    v[2].z = 512;
    v[3].x = -MAX_COORD;
    v[3].y = MAX_COORD;
    v[3].z = 512;

    dglSetVertexColor(&v[0], sky->skycolor[0], 4);

    dglTriangle(0, 1, 3);
    dglTriangle(2, 3, 1);
    dglDrawGeometry(4, v);

    //
    // draw horizon wall
    //
    v[0].x = -MAX_COORD;
    v[0].y = 512;
    v[0].z = 12;
    v[1].x = -MAX_COORD;
    v[1].y = 512;
    v[1].z = 512;
    v[2].x = MAX_COORD;
    v[2].y = 512;
    v[2].z = 512;
    v[3].x = MAX_COORD;
    v[3].y = 512;
    v[3].z = 12;

    dglSetVertexColor(&v[0], sky->skycolor[1], 1);
    dglSetVertexColor(&v[1], sky->skycolor[0], 1);
    dglSetVertexColor(&v[2], sky->skycolor[0], 1);
    dglSetVertexColor(&v[3], sky->skycolor[1], 1);

    dglTriangle(0, 1, 2);
    dglTriangle(3, 0, 2);
    dglDrawGeometry(4, v);
    dglEnable(GL_TEXTURE_2D);
    dglPopMatrix();

    //
    // setup model matrix for clouds
    //
    dglPushMatrix();
    dglRotatef(-TRUEANGLES(viewpitch), 1.0f, 0.0f, 0.0f);

    dglDisable(GL_DEPTH_TEST);
    dglDepthMask(GL_FALSE);

    dglRotatef(-TRUEANGLES(viewangle) + 90.0f, 0.0f, 0.0f, 1.0f);

    //
    // bind cloud texture and set blending
    //
    GL_SetTextureUnit(0, true);
    GL_BindGfxTexture(lumpinfo[skypicnum].name, true);

    GLint oldWrapS_cloud = 0, oldWrapT_cloud = 0;
    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, &oldWrapS_cloud);
    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, &oldWrapT_cloud);
    dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    GLint old2DMin_cloud = 0, old2DMag_cloud = 0;
    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, &old2DMin_cloud);
    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, &old2DMag_cloud);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (int)r_skyFilter.value == 0 ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (int)r_skyFilter.value == 0 ? GL_LINEAR : GL_NEAREST);

    I_ShaderSetUseTexture(1);

    GLint tw = 0, th = 0;
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &tw);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &th);
    I_ShaderSetTextureSize(tw, th);

    GL_SetState(GLSTATE_BLEND, 1);
    dglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_PRIMARY_COLOR);
    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_TEXTURE);


    // atsb:
    // draw first cloud layer
    //
    v[0].tu = sky_cloudpan1;
    v[0].tv = sky_cloudpan1;
    v[1].tu = 16 + sky_cloudpan1;
    v[1].tv = sky_cloudpan1;
    v[2].tu = 16 + sky_cloudpan1;
    v[2].tv = 16 + sky_cloudpan1;
    v[3].tu = sky_cloudpan1;
    v[3].tv = 16 + sky_cloudpan1;
    v[0].x = -MAX_COORD;
    v[0].y = -MAX_COORD;
    v[0].z = 768;
    v[1].x = MAX_COORD;
    v[1].y = -MAX_COORD;
    v[1].z = 768;
    v[2].x = MAX_COORD;
    v[2].y = MAX_COORD;
    v[2].z = 768;
    v[3].x = -MAX_COORD;
    v[3].y = MAX_COORD;
    v[3].z = 768;

    dglSetVertexColor(&v[0], sky->skycolor[2], 4);

    dglTriangle(0, 1, 3);
    dglTriangle(2, 3, 1);
    dglDrawGeometry(4, v);

    // atsb:
    // draw second cloud layer
    //
    // preserve color and xy and just update
    // uv coords and z height
    //
    v[0].tu = sky_cloudpan2;
    v[0].tv = sky_cloudpan2;
    v[1].tu = 32 + sky_cloudpan2;
    v[1].tv = sky_cloudpan2;
    v[2].tu = 32 + sky_cloudpan2;
    v[2].tv = 32 + sky_cloudpan2;
    v[3].tu = sky_cloudpan2;
    v[3].tv = 32 + sky_cloudpan2;
    v[0].z = v[1].z = v[2].z = v[3].z = 1024;

    dglTriangle(0, 1, 3);
    dglTriangle(2, 3, 1);
    dglDrawGeometry(4, v);

    //


    dglPopMatrix();
    GL_SetState(GLSTATE_BLEND, 0);

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_PRIMARY_COLOR);

    dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, oldWrapS_cloud);
    dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, oldWrapT_cloud);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, old2DMin_cloud);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, old2DMag_cloud);
    dglDepthMask(GL_TRUE);
    dglEnable(GL_DEPTH_TEST);

    I_ShaderSetUseTexture(1);
    I_ShaderSetTextureSize(0, 0);

    I_ShaderBind();

#undef SKYBOX_SETALPHA
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

    I_ShaderUnBind();

    gfxLmp = GL_BindGfxTexture(lumpinfo[lump].name, true);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_PRIMARY_COLOR);
    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_TEXTURE);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.5f);

    GL_SetOrtho(1);

    dglMatrixMode(GL_MODELVIEW);
    dglPushMatrix();
    dglLoadIdentity();

    height = gfxheight[gfxLmp];
    lumpheight = gfxorigheight[gfxLmp];

    dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    SKYVIEWPOS(viewangle, 1, pos1);

    width = (float)SCREENWIDTH / (float)gfxwidth[gfxLmp];
    row = (float)lumpheight / (float)height;

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

//
// R_DrawNamedSkyOnce
// Draw a backdrop by lump name exactly once across the screen,
// with configurable scaling/alignment controlled by BACKPIC macros.
//

static void R_DrawNamedSkyOnce(const char* lumpname, int offset, int drawHeight) {
    float pos1;
    int gfxLmp;
    int height, lumpheight;
    float row;
    float width;

    I_ShaderUnBind();

    gfxLmp = GL_BindGfxTexture(lumpname, true);
    if (gfxLmp < 0) {
        return;
    }

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_PRIMARY_COLOR);
    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_TEXTURE);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, BACKPIC_ALPHA_THRESHOLD);

    GL_SetOrtho(1);

    dglMatrixMode(GL_MODELVIEW);
    dglPushMatrix();
    dglLoadIdentity();

    dglEnable(GL_TEXTURE_2D);

    height = gfxheight[gfxLmp];
    lumpheight = gfxorigheight[gfxLmp];

    dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
#if BACKPIC_WRAP_T_CLAMP
    dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
#else
    dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
#endif

    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, &oldMin);
    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, &oldMax);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    SKYVIEWPOS(viewangle, 1, pos1);

    // atsb: horizontal cycles across the screen
    width = (BACKPIC_U_CYCLES <= 0.0f) ? 1.0f : BACKPIC_U_CYCLES;
    row = (float)lumpheight / (float)height;

    while (pos1 >= 1.0f) pos1 -= 1.0f;
    while (pos1 < 0.0f)  pos1 += 1.0f;

    float effH = (float)drawHeight;
    switch (BACKPIC_HEIGHT_MODE) {
    default:
    case 0: { // legacy
        const float yBias = (float)SCREENHEIGHT * 0.5f;
        effH = (float)SCREENHEIGHT + yBias;
    } break;
    case 1: // use caller
        break;
    case 2: // scaled height
        effH = (float)drawHeight * (float)BACKPIC_HEIGHT_SCALE;
        break;
    case 3: // fixed height
        effH = (float)BACKPIC_FIXED_HEIGHT;
        break;
    case 4: // clamped height
        if (BACKPIC_MIN_HEIGHT > 0 && effH < (float)BACKPIC_MIN_HEIGHT) effH = (float)BACKPIC_MIN_HEIGHT;
        if (BACKPIC_MAX_HEIGHT > 0 && effH > (float)BACKPIC_MAX_HEIGHT) effH = (float)BACKPIC_MAX_HEIGHT;
        break;
    }

    // atsb: hacks for individual maps in PWADS that need different scaling and offsets to look normal (mostly for reloaded, thanks Atomic!)
    if (gamemap == 24 && W_CheckNumForName("MARSHE2") && effH < (float)BACKPIC_LARGE_FIXED_HEIGHT) {
        effH = (float)BACKPIC_LARGE_FIXED_HEIGHT;
    }

    if (gamemap == 25 && W_CheckNumForName("MOUNTEE") && effH < (float)BACKPIC_MEDIUM_FIXED_HEIGHT) {
        effH = (float)BACKPIC_MEDIUM_FIXED_HEIGHT;
    }

    float bottomY = (float)offset;
#if BACKPIC_HEIGHT_MODE == 0
#else
#if BACKPIC_POSITION_MODE == 1
    bottomY = (float)SCREENHEIGHT;
#elif BACKPIC_POSITION_MODE == 2
    float origh = (float)lumpheight;
    float classicBase = 160.0f - ((128.0f - origh) * 0.5f);
    bottomY = classicBase + (float)BACKPIC_BASE_BIAS;
#endif
#endif

    float topY;
    if (BACKPIC_HEIGHT_MODE == 0) {
        const float yBias = (float)SCREENHEIGHT * 0.5f;
        topY = -yBias;
    }
    else {
#if BACKPIC_ALIGNMENT == 1
        topY = bottomY;
#elif BACKPIC_ALIGNMENT == 2
        topY = bottomY - (effH * 0.5f);
#else
        topY = bottomY - effH;
#endif
    }

    topY += (float)BACKPIC_EXTRA_Y_OFFSET;

    GL_SetState(GLSTATE_BLEND, 1);
    dglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    GL_SetupAndDraw2DQuad(0, topY, SCREENWIDTH, effH,
        pos1, width + pos1, 0.006f, row, WHITE, 1);
    GL_SetState(GLSTATE_BLEND, 0);

    glDisable(GL_ALPHA_TEST);

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    // restore filtering state
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, oldMin);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, oldMax);

    dglPopMatrix();

    GL_ResetViewport();

    I_ShaderBind();
}

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
    rfloat pos = 0.0f;
    vtx_t v[4];

    I_ShaderUnBind();
    GL_SetTextureUnit(0, true);
    GL_BindGfxTexture(lumpinfo[skypicnum].name, true);

    GLint oldWrapS_cloud = 0, oldWrapT_cloud = 0;
    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, &oldWrapS_cloud);
    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, &oldWrapT_cloud);
    dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    GLint old2DMin_cloud2 = 0, old2DMag_cloud2 = 0;
    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, &old2DMin_cloud2);
    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, &old2DMag_cloud2);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (int)r_skyFilter.value == 0 ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (int)r_skyFilter.value == 0 ? GL_LINEAR : GL_NEAREST);

    pos = (TRUEANGLES(viewangle) / 360.0f) * 2.0f;

    dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    dglSetVertex(v);

    I_ShaderUnBind();
    GL_Set2DQuad(v, 0, 0, SCREENWIDTH, SCREENHEIGHT, 0, 0, 0, 0, 0);
    dglSetVertexColor(&v[0], sky->skycolor[0], 2); // top
    dglSetVertexColor(&v[2], sky->skycolor[1], 2); // bottom
    dglDisable(GL_TEXTURE_2D);
    GL_Draw2DQuad(v, true);
    dglEnable(GL_TEXTURE_2D);

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_PRIMARY_COLOR);
    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_TEXTURE);

    dglSetVertexColor(&v[0], sky->skycolor[2], 4);

    I_ShaderUnBind();
    GL_Set2DQuad(v, 0, 0, SCREENWIDTH, SCREENHEIGHT, 0, 0, 0, 0, 0);
    dglSetVertexColor(&v[0], sky->skycolor[0], 2);
    dglSetVertexColor(&v[2], sky->skycolor[1], 2);

    dglDisable(GL_TEXTURE_2D);

    GL_Draw2DQuad(v, true);

    dglEnable(GL_TEXTURE_2D);

    GL_SetTextureUnit(1, true);
    GL_SetTextureMode(GL_ADD);
    GL_UpdateEnvTexture(sky->skycolor[1]);
    GL_SetTextureUnit(0, true);

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
    dglPushMatrix();
    dglTranslated(0.0f, 0.0f, -1.0f);
    dglTriangle(0, 1, 2);
    dglTriangle(3, 2, 1);
    dglDrawGeometry(4, v);
    dglPopMatrix();
    dglDisable(GL_BLEND);

    dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, oldWrapS_cloud);
    dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, oldWrapT_cloud);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, old2DMin_cloud2);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, old2DMag_cloud2);

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

    fireLump = W_GetNumForName("FIRE") - g_start;
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
    float pos1;
    vtx_t v[4];
    dtexture t = gfxptr[fireLump];
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
        dglGenTextures(1, &gfxptr[fireLump]);
    }

    dglBindTexture(GL_TEXTURE_2D, gfxptr[fireLump]);
    GL_CheckFillMode();
    GL_SetTextureFilter();

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

    if (r_skybox <= 0) {
        SKYVIEWPOS(viewangle, 4, pos1);

        //
        // adjust UV by 0.0035f units due to the fire sky showing a
        // strip of color going along the top portion of the texture
        //
        GL_Set2DQuad(v, 0, 0, SCREENWIDTH, 120,
            pos1, 5.0f + pos1, 0.0035f, 1.0f, 0);

        dglSetVertexColor(&v[0], sky->skycolor[0], 2);
        dglSetVertexColor(&v[2], sky->skycolor[1], 2);

        GL_Draw2DQuad(v, 1);
    }
    else {
        R_DrawSkyDome(16, 1, 1024, 4096, -896, 0.0075f,
            sky->skycolor[0], sky->skycolor[1]);
    }
}

//
// R_DrawSky
//

void R_DrawSky(void) {

    if (!sky) {
        return;
    }

    // draw solid void sky if requested
    if (sky->flags & SKF_VOID) {
        R_DrawVoidSky();
        return;
    }

    if (skypicnum < 0) {
        return;
    }

    // atsb: if there's a backdrop sky, always draw it across the screen.
    if (skybackdropnum >= 0) {
        if (sky->flags & SKF_FADEBACK) {
            R_DrawTitleSky();
            return;
        }

        if (sky->flags & SKF_BACKGROUND) {
            float h, origh, base, offset;
            int domeheight;
            int l;

            GL_SetTextureUnit(0, true);
            l = GL_BindGfxTexture(lumpinfo[skybackdropnum].name, true);
            if (l >= 0) {
                origh = (float)gfxorigheight[l];
                h = (float)gfxheight[l];
                base = 160.0f - ((128 - origh) / 2.0f);
                domeheight = (int)(base / (origh / h));
                offset = (float)domeheight - base - 16.0f;

                if (sky->flags & SKF_CLOUD) {
                    R_DrawClouds();
                }
                R_DrawNamedSkyOnce(lumpinfo[skybackdropnum].name, (int)offset, domeheight);
                if (sky->flags & SKF_FIRE) {
                    R_DrawFire();
                }
                return;
            }
        }
    }

    // Fallback
    R_DrawNamedSkyOnce(lumpinfo[skypicnum].name, 0, SCREENHEIGHT);
    if (sky->flags & SKF_FIRE) {
        R_DrawFire();
    }
    return;

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
