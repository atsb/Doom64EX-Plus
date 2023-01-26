// Emacs style mode select   -*- C -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 2005 Simon Howard
// Copyright(C) 2007-2014 Samuel Villarreal
// Copyright(C) 2022 André Guilherme 
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
//
// DESCRIPTION:
//    SDL Input
//
//-----------------------------------------------------------------------------

#ifdef __OpenBSD__
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif
#include "doomtype.h"
////////////Input//////////////

extern int UseMouse[2];
extern int UseJoystick;
extern int mouse_x;
extern int mouse_y;

int I_MouseAccel(int val);
void I_MouseAccelChange(void);

void ISDL_RegisterKeyCvars(void);

static void I_GetEvent(SDL_Event* Event);
static void I_ReadMouse(void);
static void I_InitInputs(void);

void I_StartTic(void);
void I_FinishUpdate(void);
int I_ShutdownWait(void);
void I_CenterMouse(void);
boolean I_UpdateGrab(void);
