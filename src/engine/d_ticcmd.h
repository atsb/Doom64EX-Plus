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

#ifndef __D_TICCMD__
#define __D_TICCMD__

#include "doomtype.h"

// The data sampled per tick (single player)
// and transmitted to other peers (multiplayer).
// Mainly movements/button commands per game tick,
// plus a checksum for internal state consistency.
typedef struct {
    signed char	forwardmove;	// *2048 for move
    signed char	sidemove;	// *2048 for move
    short       angleturn;    // <<16 for angle delta
    short       pitch;
    byte        consistency;    // checks for net game
    byte        chatchar;
    byte        buttons;
    byte        buttons2;
} ticcmd_t;

#endif
