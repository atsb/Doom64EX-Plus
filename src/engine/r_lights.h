// Emacs style mode select   -*- C -*-
//-----------------------------------------------------------------------------
//
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

#ifndef D3DR_LIGHTS_H
#define D3DR_LIGHTS_H

#include "gl_main.h"
#include "t_bsp.h"

enum {
	LIGHT_FLOOR,
	LIGHT_CEILING,
	LIGHT_THING,
	LIGHT_UPRWALL,
	LIGHT_LWRWALL
};

extern rcolor    bspColor[5];

rcolor R_GetSectorLight(byte alpha, word ptr);
void R_SetLightFactor(float lightfactor);
void R_RefreshBrightness(void);
void R_LightToVertex(vtx_t* v, int idx, word c);
void R_SetSegLineColor(seg_t* line, vtx_t* v, byte side);

#endif
