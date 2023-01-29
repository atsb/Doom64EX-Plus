// Emacs style mode select   -*- C -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 2007-2012 Samuel Villarreal
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
// 02111-1307, USA.
//
//-----------------------------------------------------------------------------

#ifndef __GL_MAIN_H__
#define __GL_MAIN_H__
#include "doomtype.h"

#ifdef __APPLE__
typedef void* GLhandleARB;
#else
typedef unsigned int GLhandleARB;
#endif
typedef GLhandleARB    rhandle;

extern int gl_max_texture_units;
extern int gl_max_texture_size;
extern boolean gl_has_combiner;
typedef struct {
	float    x;
	float    y;
	float    z;
	float    tu;
	float    tv;
	unsigned char r;
	unsigned char g;
	unsigned char b;
	unsigned char a;
} vtx_t;

#define MAXSPRPALSETS       4

typedef struct {
	unsigned char r;
	unsigned char g;
	unsigned char b;
	unsigned char a;
} dPalette_t;

#ifdef NIGHTDIVE

#ifndef BIT
#define BIT(num) (1<<(num))
#endif

typedef enum
{
    DGLSTATE_BLEND = 0,
    DGLSTATE_CULL,
    DGLSTATE_TEXTURE0,
    DGLSTATE_TEXTURE1,
    DGLSTATE_TEXTURE2,
    DGLSTATE_TEXTURE3,
    GLSTATE_DEPTHTEST,
    GLSTATE_STENCILTEST,
    GLSTATE_SCISSOR,
    GLSTATE_ALPHATEST,
    GLSTATE_TEXGEN_S,
    GLSTATE_TEXGEN_T,
    GLSTATE_FOG,
    NUMGLSTATES
} glState_t;

typedef enum
{
    GLFUNC_LEQUAL = 0,
    GLFUNC_GEQUAL,
    GLFUNC_EQUAL,
    GLFUNC_NOTEQUAL,
    GLFUNC_GREATER,
    GLFUNC_LESS,
    GLFUNC_ALWAYS,
    GLFUNC_NEVER,
} glFunctions_t;

typedef enum {
    GLCULL_FRONT = 0,
    GLCULL_BACK
} glCullType_t;

typedef enum
{
    GLPOLY_FILL = 0,
    GLPOLY_LINE
} glPolyMode_t;

typedef enum
{
    GLSRC_ZERO = 0,
    GLSRC_ONE,
    GLSRC_DST_COLOR,
    GLSRC_ONE_MINUS_DST_COLOR,
    GLSRC_SRC_ALPHA,
    GLSRC_ONE_MINUS_SRC_ALPHA,
    GLSRC_DST_ALPHA,
    GLSRC_ONE_MINUS_DST_ALPHA,
    GLSRC_ALPHA_SATURATE,
} glSrcBlend_t;

typedef enum
{
    GLDST_ZERO = 0,
    GLDST_ONE,
    GLDST_SRC_COLOR,
    GLDST_ONE_MINUS_SRC_COLOR,
    GLDST_SRC_ALPHA,
    GLDST_ONE_MINUS_SRC_ALPHA,
    GLDST_DST_ALPHA,
    GLDST_ONE_MINUS_DST_ALPHA,
} glDstBlend_t;

typedef enum
{
    GLCB_COLOR = BIT(0),
    GLCB_DEPTH = BIT(1),
    GLCB_STENCIL = BIT(2),
    GLCB_ALL = (GLCB_COLOR | GLCB_DEPTH | GLCB_STENCIL)
} glClearBit_t;

#define MAX_TEXTURE_UNITS 4

typedef enum
{
    TC_CLAMP = 0,
    TC_CLAMP_BORDER,
    TC_REPEAT,
    TC_MIRRORED
} texClampMode_t;

typedef enum
{
    TF_LINEAR = 0,
    TF_NEAREST
} texFilterMode_t;

typedef enum {
    TCR_RGB = 0,
    TCR_RGBA
} texColorMode_t;

typedef struct
{
    int             currentTexture;
    int             environment;
} texUnit_t;

typedef struct
{
    unsigned int    glStateBits;
    int             depthFunction;
    glSrcBlend_t    blendSrc;
    glDstBlend_t    blendDest;
    glCullType_t    cullType;
    glPolyMode_t    polyMode;
    int             depthMask;
    int             colormask;
    glFunctions_t   alphaFunction;
    float           alphaFuncThreshold;
    int             currentUnit;
    rhandle         currentProgram;
    int        currentFBO;
    texUnit_t       textureUnits[MAX_TEXTURE_UNITS];
    int             numStateChanges;
    int             numTextureBinds;
    int             numDrawnVertices;
    unsigned int         drawBuffer;
    unsigned int          readBuffer;
} rbState_t;

typedef struct {
    int                 width;
    int                 height;
    int                 origwidth;
    int                 origheight;
    texClampMode_t      clampMode;
    texFilterMode_t     filterMode;
    texColorMode_t      colorMode;
    int                 texid;
} rbTexture_t;

extern rbState_t rbState;
extern rbTexture_t rbTexture;

#endif

#define GLSTATE_BLEND       0
#define GLSTATE_CULL        1
#define GLSTATE_TEXTURE0    2
#define GLSTATE_TEXTURE1    3
#define GLSTATE_TEXTURE2    4
#define GLSTATE_TEXTURE3    5

extern int ViewWidth;
extern int ViewHeight;
extern int ViewWindowX;
extern int ViewWindowY;

#define FIELDOFVIEW         2048            // Fineangles in the video_width wide window.
#define ALPHACLEARGLOBAL    0.01f
#define ALPHACLEARTEXTURE   0.8f
#define MAX_COORD           32767.0f

#define TESTALPHA(x)        ((unsigned char)((x >> 24) & 0xff) < 0xff)

extern boolean usingGL;

boolean GL_CheckExtension(const char* ext);
void* GL_RegisterProc(const char* address);
void GL_Init(void);
void GL_ClearView(unsigned int clearcolor);
void GL_CheckFillMode(void);
void GL_SwapBuffers(void);
unsigned char* GL_GetScreenBuffer(int x, int y, int width, int height);
void GL_SetTextureFilter(void);
void GL_SetOrtho(boolean stretch);
void GL_ResetViewport(void);
void GL_SetOrthoScale(float scale);
float GL_GetOrthoScale(void);
void GL_SetState(int bit, boolean enable);
void GL_SetDefaultCombiner(void);
void GL_Set2DQuad(vtx_t* v, float x, float y, int width, int height,
	float u1, float u2, float v1, float v2, unsigned int c);
void GL_Draw2DQuad(vtx_t* v, boolean stretch);
void GL_SetupAndDraw2DQuad(float x, float y, int width, int height,
	float u1, float u2, float v1, float v2, unsigned int c, boolean stretch);

#endif
