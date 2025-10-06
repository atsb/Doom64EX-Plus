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

#ifndef __G_DEMO_H__
#define __G_DEMO_H__

#include "doomtype.h"
#include "doomdef.h"
#include "d_ticcmd.h"

#define DEMOMARKER      0x80

boolean G_CheckDemoStatus(void);

void G_RecordDemo(const char* name);
void G_PlayDemo(const char* name);
void G_ReadDemoTiccmd(ticcmd_t* cmd);
void G_WriteDemoTiccmd(ticcmd_t* cmd);

extern char             demoname[256];  // name of demo lump
extern boolean         demorecording;  // currently recording a demo
extern boolean         demoplayback;   // currently playing a demo
extern boolean         netdemo;
extern byte* demobuffer;
extern byte* demo_p;
extern byte* demoend;
extern boolean         singledemo;
extern boolean         endDemo;        // signal recorder to stop on next tick
extern boolean         iwadDemo;       // hide hud, end playback after one level

/* VANILLA */
int G_PlayDemoPtr(int skill, int map); // 800049D0
int D_RunDemo(char* name, skill_t skill, int map); // 8002B2D0
#endif
