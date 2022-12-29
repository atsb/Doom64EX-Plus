// Emacs style mode select   -*- C -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 1993-1997 Id Software, Inc.
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

#ifndef __G_DEMO_H__
#define __G_DEMO_H__

#define DEMOMARKER      0x80

boolean G_CheckDemoStatus(void);

void G_RecordDemo(const int8_t* name);
void G_PlayDemo(const int8_t* name);
void G_ReadDemoTiccmd(ticcmd_t* cmd);
void G_WriteDemoTiccmd(ticcmd_t* cmd);

extern int8_t             demoname[256];  // name of demo lump
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
int D_RunDemo(int8_t* name, skill_t skill, int map); // 8002B2D0
#endif
