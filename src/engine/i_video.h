// Emacs style mode select   -*- C -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 1993-1997 Id Software, Inc.
// Copyright(C) 2005 Simon Howard
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

#ifndef __I_VIDEO_H__
#define __I_VIDEO_H__

#ifdef __OpenBSD__
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif

#include "d_event.h"

////////////Video///////////////

extern SDL_Surface* screen;
extern SDL_Window* window;
void I_InitVideo(void);
void I_InitScreen(void);
void I_ShutdownVideo(void);
void V_RegisterCvars();

#endif
