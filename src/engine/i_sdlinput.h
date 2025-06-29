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
//
// DESCRIPTION:
//    SDL Input
//
//-----------------------------------------------------------------------------

#include <SDL3/SDL.h>
#include "doomtype.h"
////////////Input//////////////

extern int UseMouse[2];
extern int UseJoystick;
extern float mouse_x;
extern float mouse_y;

float I_MouseAccel(float val);
void I_MouseAccelChange(void);

void ISDL_RegisterKeyCvars(void);

void I_GetEvent(SDL_Event* Event);
void I_ReadMouse(void);
void I_InitInputs(void);

void I_StartTic(void);
void I_FinishUpdate(void);
int I_ShutdownWait(void);
void I_CenterMouse(void);
boolean I_UpdateGrab(void);
