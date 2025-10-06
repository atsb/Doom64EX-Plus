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

#ifndef __WI_STUFF__
#define __WI_STUFF__

// Called by main loop, animate the intermission.
int WI_Ticker(void);

void WI_Stop(void);

// Called by main loop,
// draws the intermission directly into the screen buffer.
void WI_Drawer(void);

// Setup for an intermission screen.
void WI_Start(void);

#endif
