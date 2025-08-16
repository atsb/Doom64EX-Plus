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

#ifndef _R_THINGS_H_
#define _R_THINGS_H_

#include <stdint.h> // for intptr_t

#include "t_bsp.h"
#include "d_player.h"

typedef struct {
	mobj_t* spr;
	fixed_t dist;
	float   x;
	float   y;
	float   z;
} visspritelist_t;

void R_InitSprites(char** namelist);
void R_AddSprites(subsector_t* sub);
void R_SetupSprites(void);
void R_ClearSprites(void);
void R_RenderPlayerSprites(player_t* player);
void R_DrawThingBBox(void);

extern spritedef_t* spriteinfo;
extern intptr_t            numsprites;

#endif
