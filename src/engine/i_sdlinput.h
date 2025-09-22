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

#ifndef __I_SDLINPUT__
#define __I_SDLINPUT__

#include <SDL3/SDL.h>

#include "doomtype.h"

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
void I_SetMousePos(float x, float y);
boolean I_UpdateGrab(void);

const char* I_KeycodeToName_All(int keycode);
int I_NameToKeycode_All(const char* name);
void I_RegisterGamepadKeyNames(void);

typedef struct {
	SDL_Gamepad* gamepad;
	SDL_Joystick* joy;
	SDL_JoystickID active_id;

	bool player_forward, player_backwards, player_left, player_right;
	bool player_fire, player_next_weapon, player_previous_weapon, player_use, player_pause, player_run, player_automap;

	bool mouse_up, mouse_down, mouse_left, mouse_right;
	bool mouse_accept, mouse_back, mouse_scroll_up, mouse_scroll_down;
	unsigned int right_arrow_key_up, right_arrow_key_down, right_arrow_key_left, right_arrow_key_right;
	float gamepad_look_fx, gamepad_look_fy, gamepad_look_dx, gamepad_look_dy;

	bool init;
} gamepad64_t;

#endif
