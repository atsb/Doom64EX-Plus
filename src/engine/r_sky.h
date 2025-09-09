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

#ifndef D3DR_SKY_H
#define D3DR_SKY_H

#include "p_setup.h"

extern skydef_t* sky;
extern int          skypicnum;
extern int          skybackdropnum;
extern int          thunderCounter;
extern int          lightningCounter;
extern int          thundertic;
extern bool         skyfadeback;

// Needed to store the number of the dummy sky flat.
// Used for rendering, as well as tracking projectiles etc.
extern int          skyflatnum;

extern byte* fireBuffer;
extern dPalette_t   firePal16[256];
extern int          fireLump;

void R_SkyTicker(void);
void R_DrawSky(void);
void R_InitFire(void);

#endif
