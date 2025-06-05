// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 2014 Zohar Malamant
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

#include <SDL.h>
#include <SDL_gamepad.h>

#include "doomdef.h"
#include "con_cvar.h"
#include "d_main.h"
#include "i_xinput.h"
#include "z_zone.h"

CVAR(i_joysensx, 3.0f);
CVAR(i_joysensy, 3.0f);
CVAR(i_joytwinstick, 0);

typedef int (*keyconv_t)(int);

typedef struct {
	int16_t device_id;
	SDL_Gamepad* device;

	char* name;

	keyconv_t keyconv;

	int num_axes;
	int num_hats;
} joy_t;

static joy_t* joy = NULL;
static int num_joy = 0;

//
// keyconv_Playstation4
//

static int keyconv_Playstation4(int key) {
	int ch = -1;

	if (key == SDL_GAMEPAD_BUTTON_DPAD_UP)
		ch = KEY_UPARROW;
	else if (key == SDL_GAMEPAD_BUTTON_DPAD_RIGHT)
		ch = KEY_RIGHTARROW;
	else if (key == SDL_GAMEPAD_BUTTON_DPAD_DOWN)
		ch = KEY_DOWNARROW;
	else if (key == SDL_GAMEPAD_BUTTON_DPAD_LEFT)
		ch = KEY_LEFTARROW;
	else if (key == SDL_GAMEPAD_BUTTON_START)
		ch = KEY_ESCAPE;
	else if (key == SDL_GAMEPAD_BUTTON_BACK)
		ch = KEY_BACKSPACE;
	else if (key == SDL_GAMEPAD_BUTTON_LABEL_A)
		ch = KEY_ENTER;

	return ch;
}

typedef struct {
	const char* name;
	keyconv_t func;
} keyconvstruct_t;

static keyconvstruct_t keyconv[] = {
	{ "PS4 Controller", keyconv_Playstation4 },
};

static int num_keyconv = sizeof(keyconv) / sizeof(keyconv[0]);

//
// JoystickCloseAll
//

static void JoystickCloseAll()
{
	int i;

	for (i = 0; i < num_joy; i++) {
		SDL_CloseGamepad(joy[i].device);
		if (joy[i].name)
			Z_Free(joy[i].name);
	}
	if (joy) {
		Z_Free(joy);
		joy = 0;
	}
}

//
// JoystickClose
//

static void JoystickClose(int device_id)
{
	int i;

	for (i = 0; i < num_joy; i++) {
		if (joy[i].device_id == device_id) {
			I_Printf("Controller %d \"%s\" disconnected...\n", joy[i].device_id, joy[i].name);

			SDL_CloseGamepad(joy[i].device);
			Z_Free(joy[i].name);
			break;
		}
	}

	// Joystick not found.
	if (i == num_joy)
		return;

	num_joy--;
	for (; i < num_joy; i++) {
		joy[i] = joy[i + 1];
	}

	Z_ReallocV(joy, num_joy, PU_STATIC, NULL);
}

//
// JoystickOpen
//

static void JoystickOpen(int device_id)
{
	int i;
	joy_t j;
	const char* name;

	// Check if the device is a GameController
	if (!SDL_IsGamepad(device_id))
		return;

	j.device = SDL_OpenGamepad(device_id);

	if (!j.device)
		return;

	j.device_id = device_id;
	j.keyconv = NULL;
	j.num_axes = SDL_GAMEPAD_AXIS_COUNT;  // SDL_GameController has a fixed number of axes
	j.num_hats = 0;  // Game controllers usually don't use hats

	name = SDL_GetGamepadName(j.device);

	if (name) {
		j.name = Z_Malloc(dstrlen(name) + 1, PU_STATIC, NULL);
		dstrcpy(j.name, name);

		for (i = 0; i < num_keyconv; i++) {
			if (dstrcmp(j.name, keyconv[i].name) == 0) {
				j.keyconv = keyconv[i].func;
				break;
			}
		}
	}
	else {
		j.name = NULL;
	}

	num_joy++;
	Z_ReallocV(joy, num_joy, PU_STATIC, NULL);

	joy[num_joy - 1] = j;

	if (j.keyconv) {
		I_Printf("Supported controller %d \"%s\" connected...\n", j.device_id, j.name);
	}
	else {
		I_Printf("Controller %d \"%s\" connected...\n", j.device_id, j.name);
	}
}

//
// I_InitJoystick
//

void I_InitJoystick(void)
{
	// sad face :(
}

//
// I_ShutdownJoystick
//

void I_ShutdownJoystick(void)
{
	JoystickCloseAll();
}

//
// I_ReadJoystick
//

void I_ReadJoystick(void)
{
	int16_t lx, ly, rx, ry;
	event_t event;

	if (!num_joy)
		return;

	lx = SDL_GetGamepadAxis(joy[0].device, SDL_GAMEPAD_AXIS_LEFTX);
	ly = SDL_GetGamepadAxis(joy[0].device, SDL_GAMEPAD_AXIS_LEFTY);
	rx = SDL_GetGamepadAxis(joy[0].device, SDL_GAMEPAD_AXIS_RIGHTX);
	ry = SDL_GetGamepadAxis(joy[0].device, SDL_GAMEPAD_AXIS_RIGHTY);

	event.type = ev_gamepad;
	event.data1 = lx;
	event.data2 = ly;
	event.data3 = rx;
	event.data4 = ry;
	D_PostEvent(&event);
}

//
// I_JoystickName
//

const char* I_JoystickName(void)
{
	if (!num_joy || !joy[0].name)
		return "None";

	return joy[0].name;
}

//
// I_JoystickToKey
//

int I_JoystickToKey(int key)
{
	if (!num_joy || !joy[0].keyconv)
		return -1;

	return joy[0].keyconv(key);
}

//
// I_JoystickEvent
//

char I_JoystickEvent(const SDL_Event* Event)
{
	event_t event;
	switch (Event->type) {
	case SDL_EVENT_GAMEPAD_REMOVED:
		JoystickClose(Event->cdevice.which);
		break;

	case SDL_EVENT_GAMEPAD_ADDED:
		JoystickOpen(Event->cdevice.which);
		break;

	case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
		I_Printf("Controller button %d pressed\n", Event->button.button);
		event.type = ev_gamepaddown;
		event.data1 = Event->button.button;
		D_PostEvent(&event);
		break;

	case SDL_EVENT_GAMEPAD_BUTTON_UP:
		event.type = ev_gamepadup;
		event.data1 = Event->button.button;
		D_PostEvent(&event);
		break;

	default:
		return false;
		break;
	}

	return true;
}

//
// I_RegisterJoystickCvars
//

void I_RegisterJoystickCvars(void)
{
	CON_CvarRegister(&i_joytwinstick);
	CON_CvarRegister(&i_joysensx);
	CON_CvarRegister(&i_joysensy);
}
