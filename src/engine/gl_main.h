// Emacs style mode select   -*- C -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 1993-1997 Id Software, Inc.
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

#ifndef __GL_MAIN_H__
#define __GL_MAIN_H__

#include <SDL3/SDL_opengl.h>
#include "doomtype.h"

typedef GLuint        dtexture;
typedef GLfloat        rfloat;
typedef GLuint        rcolor;
typedef GLuint        rbuffer;
typedef GLhandleARB    rhandle;

extern int gl_max_texture_units;
extern int gl_max_texture_size;
extern boolean gl_has_combiner;

typedef struct {
	rfloat    x;
	rfloat    y;
	rfloat    z;
	rfloat    tu;
	rfloat    tv;
	byte r;
	byte g;
	byte b;
	byte a;
} vtx_t;

#define MAXSPRPALSETS       4

typedef struct {
	byte r;
	byte g;
	byte b;
	byte a;
} dPalette_t;

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

#define TESTALPHA(x)        ((byte)((x >> 24) & 0xff) < 0xff)

extern boolean usingGL;
extern boolean widescreen;

boolean GL_CheckExtension(const char* ext);
void* GL_RegisterProc(const char* address);
void GL_Init(void);
void GL_ClearView(rcolor clearcolor);
boolean GL_GetBool(int x);
void GL_CheckFillMode(void);
void GL_SwapBuffers(void);
byte* GL_GetScreenBuffer(int x, int y, int width, int height);
void GL_SetTextureFilter(void);
void GL_SetOrtho(boolean stretch);
void GL_ResetViewport(void);
void GL_SetOrthoScale(float scale);
float GL_GetOrthoScale(void);
void GL_SetState(int bit, boolean enable);
void GL_SetDefaultCombiner(void);
void GL_SetColorScale(void);
void GL_Set2DQuad(vtx_t* v, float x, float y, int width, int height,
	float u1, float u2, float v1, float v2, rcolor c);
void GL_Draw2DQuad(vtx_t* v, boolean stretch);
void GL_SetupAndDraw2DQuad(float x, float y, int width, int height,
	float u1, float u2, float v1, float v2, rcolor c, boolean stretch);
#endif
