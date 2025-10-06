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

#ifndef __GL_DRAW_H__
#define __GL_DRAW_H__

#include "gl_main.h"

void Draw_GfxImage(int x, int y, const char* name,
	rcolor color, boolean alpha);
void Draw_GfxImageTitle(int x, int y, const char* name,
	rcolor color, boolean alpha);
void Draw_GfxImageInter(int x, int y, const char* name,
	rcolor color, boolean alpha);
void Draw_GfxImageLegal(int x, int y, const char* name,
	rcolor color, boolean alpha);
void Draw_GfxImageIN(int x, int y, const char* name,
	rcolor color, boolean alpha);
void Draw_Sprite2D(int type, int rot, int frame, int x, int y,
	float scale, int pal, rcolor c);

#define SM_FONT1        16
#define SM_FONT2        42
#define SM_MISCFONT        10
#define SM_NUMBERS        0
#define SM_SKULLS        70
#define SM_THERMO        68
#define SM_MICONS        78

typedef struct {
	int x;
	int y;
	int w;
	int h;
} symboldata_t;

extern const symboldata_t symboldata[];

#define ST_FONTWHSIZE    8
#define ST_FONTNUMSET    32    //# of fonts per row in font pic
#define ST_FONTSTART    '!'    // the first font characters
#define ST_FONTEND        '_'    // the last font characters
#define ST_FONTSIZE        (ST_FONTEND - ST_FONTSTART + 1) // Calculate # of glyphs in font.

int Draw_Text(int x, int y, rcolor color, float scale,
	boolean wrap, const char* string, ...);
int Draw_TextSecret(int x, int y, rcolor color, float scale,
	boolean wrap, const char* string, ...);
int Center_Text(const char* string);
int Draw_BigText(int x, int y, rcolor color, const char* string);
int Draw_SmallText(int x, int y, rcolor color, const char* string);
void Draw_Number(int x, int y, int num, int type, rcolor c);
float Draw_ConsoleText(float x, float y, rcolor color,
	float scale, const char* string, ...);

#endif
