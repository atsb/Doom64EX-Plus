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

#ifndef __AMMAP_H__
#define __AMMAP_H__

CVAR_EXTERNAL(am_overlay);

// Called by main loop.
dboolean AM_Responder(event_t* ev);

// Called by main loop.
void AM_Ticker(void);

// Called by main loop,
// called instead of view drawer if automap active.
void AM_Drawer(void);

// Called to force the automap to quit
// if the level is completed while it is up.
void AM_Stop(void);

// Called on P_Start; resets automap variables
void AM_Reset(void);

void AM_RegisterCvars(void);

#endif
