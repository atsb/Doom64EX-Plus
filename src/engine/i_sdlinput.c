// Emacs style mode select   -*- C -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 2005 Simon Howard
// Copyright(C) 2007-2014 Samuel Villarreal
// Copyright(C) 2022-2023 André Guilherme
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

#include <assert.h>
#include "i_sdlinput.h"
#include "doomdef.h"
#include "m_misc.h"
#include "doomstat.h"
#include "i_system.h"
#include "i_video.h"
#include "d_main.h"
#include "con_console.h"

CVAR(i_rsticksensitivityx, 0.0080);
CVAR(i_rsticksensitivityy, 0.0080);
CVAR(i_xinputscheme, 0);
CVAR(v_msensitivityx, 5);
CVAR(v_msensitivityy, 5);
CVAR(v_macceleration, 0);
CVAR(v_mlook, 0);
CVAR(v_mlookinvert, 0);
CVAR(v_yaxismove, 0);
CVAR(v_xaxismove, 0);
CVAR_EXTERNAL(m_menumouse);

float mouse_accelfactor;

int         UseJoystick;
boolean    DigiJoy;
int         DualMouse;

boolean    MouseMode;//false=microsoft, true=mouse systems
boolean window_mouse;

signed int s_deadzone = 8000;

void s_clamp(signed int axis)
{
	if (axis < s_deadzone && axis > -s_deadzone)
	{
		axis = 0;
	}
}
SDL_GameController* s_controller;


int I_Translate_GameController(int state)
{
	int rc = 0;
	switch (state)
	{
	case SDL_CONTROLLER_BUTTON_A:
		return rc = GAMEPAD_A;
		break;
	case SDL_CONTROLLER_BUTTON_B:
		return rc = GAMEPAD_B;
		break;
	case SDL_CONTROLLER_BUTTON_X:
		return rc = GAMEPAD_X;
		break;
	case SDL_CONTROLLER_BUTTON_Y:
		return rc = GAMEPAD_Y;
		break;
	case SDL_CONTROLLER_BUTTON_BACK:
		return rc = GAMEPAD_BACK;
		break;
	case SDL_CONTROLLER_BUTTON_GUIDE:
		return rc = GAMEPAD_GUIDE;
		break;
	case SDL_CONTROLLER_BUTTON_START:
		return rc = GAMEPAD_START;
		break;
	case SDL_CONTROLLER_BUTTON_LEFTSTICK:
		return rc = GAMEPAD_LSTICK;
		break;
	case SDL_CONTROLLER_BUTTON_RIGHTSTICK:
		return rc = GAMEPAD_RSTICK;
		break;
	case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
		return rc = GAMEPAD_LSHOULDER;
		break;
	case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
		return rc = GAMEPAD_RSHOULDER;
		break;
	case SDL_CONTROLLER_BUTTON_DPAD_UP:
		return rc = GAMEPAD_DPAD_UP;
		break;
	case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
		return rc = GAMEPAD_DPAD_DOWN;
		break;
	case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
		return rc = GAMEPAD_DPAD_LEFT;
		break;
	case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
		return rc = GAMEPAD_DPAD_RIGHT;
		break;
	case SDL_CONTROLLER_BUTTON_MISC1:
		return rc = GAMEPAD_BUTTON_MISC1;
		break;
	case SDL_CONTROLLER_BUTTON_PADDLE1:
		return rc = GAMEPAD_BUTTON_PADDLE1;
		break;
	case SDL_CONTROLLER_BUTTON_PADDLE2:
		return rc = GAMEPAD_BUTTON_PADDLE2;
		break;
	case SDL_CONTROLLER_BUTTON_PADDLE3:
		return rc = GAMEPAD_BUTTON_PADDLE3;
		break;
	case SDL_CONTROLLER_BUTTON_PADDLE4:
		return rc = GAMEPAD_BUTTON_PADDLE4;
		break;
	case SDL_CONTROLLER_BUTTON_TOUCHPAD:
		return rc = GAMEPAD_BUTTON_TOUCHPAD;
		break;
	default:
		rc = state;
		break;
	}
	return rc;
}

//
//I_InitEvent
//
void I_InitEvent()
{
	if (SDL_Init(SDL_INIT_EVENTS) < 0)
	{
		CON_Printf(RED, "Failed to initialize the SDL2 Events SDL Error: %s", SDL_GetError());
	}
}

//
//	I_InitGameController
//
void I_InitGameController()
{
	if (SDL_Init(SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER) < 0)
	{
		CON_Printf(RED, "Failed to initialize the SDL2 Game Controller API, SDL Error: %s", SDL_GetError());
		return;
	}
}

//
// I_TranslateKey
//
static int I_TranslateKey(const int key) {
	int rc = 0;

	switch (key) {
	case SDLK_LEFT:
		rc = KEY_LEFTARROW;
		break;
	case SDLK_RIGHT:
		rc = KEY_RIGHTARROW;
		break;
	case SDLK_DOWN:
		rc = KEY_DOWNARROW;
		break;
	case SDLK_UP:
		rc = KEY_UPARROW;
		break;
	case SDLK_ESCAPE:
		rc = KEY_ESCAPE;
		break;
	case SDLK_RETURN:
		rc = KEY_ENTER;
		break;
	case SDLK_TAB:
		rc = KEY_TAB;
		break;
	case SDLK_F1:
		rc = KEY_F1;
		break;
	case SDLK_F2:
		rc = KEY_F2;
		break;
	case SDLK_F3:
		rc = KEY_F3;
		break;
	case SDLK_F4:
		rc = KEY_F4;
		break;
	case SDLK_F5:
		rc = KEY_F5;
		break;
	case SDLK_F6:
		rc = KEY_F6;
		break;
	case SDLK_F7:
		rc = KEY_F7;
		break;
	case SDLK_F8:
		rc = KEY_F8;
		break;
	case SDLK_F9:
		rc = KEY_F9;
		break;
	case SDLK_F10:
		rc = KEY_F10;
		break;
	case SDLK_F11:
		rc = KEY_F11;
		break;
	case SDLK_F12:
		rc = KEY_F12;
		break;
	case SDLK_BACKSPACE:
		rc = KEY_BACKSPACE;
		break;
	case SDLK_DELETE:
		rc = KEY_DEL;
		break;
	case SDLK_INSERT:
		rc = KEY_INSERT;
		break;
	case SDLK_PAGEUP:
		rc = KEY_PAGEUP;
		break;
	case SDLK_PAGEDOWN:
		rc = KEY_PAGEDOWN;
		break;
	case SDLK_HOME:
		rc = KEY_HOME;
		break;
	case SDLK_END:
		rc = KEY_END;
		break;
	case SDLK_PAUSE:
		rc = KEY_PAUSE;
		break;
	case SDLK_EQUALS:
		rc = KEY_EQUALS;
		break;
	case SDLK_MINUS:
		rc = KEY_MINUS;
		break;
	case SDLK_KP_0:
		rc = KEY_KEYPAD0;
		break;
	case SDLK_KP_1:
		rc = KEY_KEYPAD1;
		break;
	case SDLK_KP_2:
		rc = KEY_KEYPAD2;
		break;
	case SDLK_KP_3:
		rc = KEY_KEYPAD3;
		break;
	case SDLK_KP_4:
		rc = KEY_KEYPAD4;
		break;
	case SDLK_KP_5:
		rc = KEY_KEYPAD5;
		break;
	case SDLK_KP_6:
		rc = KEY_KEYPAD6;
		break;
	case SDLK_KP_7:
		rc = KEY_KEYPAD7;
		break;
	case SDLK_KP_8:
		rc = KEY_KEYPAD8;
		break;
	case SDLK_KP_9:
		rc = KEY_KEYPAD9;
		break;
	case SDLK_KP_PLUS:
		rc = KEY_KEYPADPLUS;
		break;
	case SDLK_KP_MINUS:
		rc = KEY_KEYPADMINUS;
		break;
	case SDLK_KP_DIVIDE:
		rc = KEY_KEYPADDIVIDE;
		break;
	case SDLK_KP_MULTIPLY:
		rc = KEY_KEYPADMULTIPLY;
		break;
	case SDLK_KP_ENTER:
		rc = KEY_KEYPADENTER;
		break;
	case SDLK_KP_PERIOD:
		rc = KEY_KEYPADPERIOD;
		break;
	case SDLK_LSHIFT:
	case SDLK_RSHIFT:
		rc = KEY_RSHIFT;
		break;
	case SDLK_LCTRL:
	case SDLK_RCTRL:
		rc = KEY_RCTRL;
		break;
	case SDLK_LALT:

	case SDLK_RALT:
		rc = KEY_RALT;
		break;
	default:
		rc = key;
		break;
	}

	return rc;
}

//
// I_SDLtoDoomMouseState
//

static int I_SDLtoDoomMouseState(unsigned char buttonstate) {
	return 0
		| (buttonstate & SDL_BUTTON(SDL_BUTTON_LEFT) ? 1 : 0)
		| (buttonstate & SDL_BUTTON(SDL_BUTTON_MIDDLE) ? 2 : 0)
		| (buttonstate & SDL_BUTTON(SDL_BUTTON_RIGHT) ? 4 : 0);
}

//
// I_ReadMouse
//

static void I_ReadMouse(void) {
	int x, y;
	unsigned char btn;
	event_t ev;
	static unsigned char lastmbtn = 0;

	SDL_GetRelativeMouseState(&x, &y);
	btn = SDL_GetMouseState(&mouse_x, &mouse_y);

	if (x != 0 || y != 0 || btn || (lastmbtn != btn)) {
		ev.type = ev_mouse;
		ev.data1 = I_SDLtoDoomMouseState(btn);
		ev.data2 = x << 5;
		ev.data3 = (-y) << 5;
		ev.data4 = 0;
		D_PostEvent(&ev);
	}

	lastmbtn = btn;
}

void I_CenterMouse(void) {
	// Warp the the screen center
	SDL_WarpMouseInWindow(window, (unsigned short)(video_width / 2), (unsigned short)(video_height / 2));

	// Clear any relative movement caused by warping
	SDL_PumpEvents();
	SDL_GetRelativeMouseState(NULL, NULL);
}

//
// I_MouseAccelChange
//

void I_MouseAccelChange(void) {
	mouse_accelfactor = v_macceleration.value / 200.0f + 1.0f;
}

//
// I_MouseAccel
//

int I_MouseAccel(int val) {
	if (!v_macceleration.value) {
		return val;
	}

	if (val < 0) {
		return -I_MouseAccel(-val);
	}

	return (int)(pow((double)val, (double)mouse_accelfactor));
}

//
// I_UpdateGrab
//

boolean I_UpdateGrab(void) {
	static boolean currently_grabbed = false;
	boolean grab;

	grab = /*window_mouse &&*/ !menuactive
		&& (gamestate == GS_LEVEL)
		&& !demoplayback;

	if (grab && !currently_grabbed) {
		SDL_ShowCursor(0);
		SDL_SetRelativeMouseMode(1);
		SDL_SetWindowGrab(window, 1);
	}

	if (!InWindow || !InWindowBorderless && m_menumouse.value <= 0) {
		return true;
	}

	if (!grab && currently_grabbed) {
		SDL_SetRelativeMouseMode(0);
		SDL_SetWindowGrab(window, 0);
	}

	currently_grabbed = grab;

	return currently_grabbed;
}

//
// I_GetEvent
//

void I_GetEvent(SDL_Event* Event) {
	event_t event;
	unsigned int mwheeluptic = 0, mwheeldowntic = 0;
	unsigned int tic = gametic;
	int x, y;


	switch (Event->type) {
	case SDL_KEYDOWN:
		if (Event->key.repeat)
			break;
		event.type = ev_keydown;
		event.data1 = I_TranslateKey(Event->key.keysym.sym);
		D_PostEvent(&event);
		break;

	case SDL_KEYUP:
		event.type = ev_keyup;
		event.data1 = I_TranslateKey(Event->key.keysym.sym);
		D_PostEvent(&event);
		break;

	case SDL_CONTROLLERDEVICEADDED:
		CON_Printf(GREEN, "SDL: Controller added: %s", SDL_GameControllerNameForIndex(Event->cdevice.which));
		s_controller = SDL_GameControllerOpen(Event->cdevice.which);
		assert(s_controller);
		break;

	case SDL_CONTROLLERDEVICEREMOVED:
		SDL_GameControllerClose(s_controller);
		s_controller = NULL;
		CON_Printf(RED, "SDL: Controller removed: %s", SDL_GameControllerNameForIndex(Event->cdevice.which));
		break;

	case SDL_CONTROLLERBUTTONDOWN:
		event.type = (Event->type == SDL_CONTROLLERBUTTONDOWN) == ev_keydown;
		event.data1 = I_Translate_GameController(Event->cbutton.button);
		D_PostEvent(&event);
		break;

	case SDL_CONTROLLERBUTTONUP:
		event.type = (Event->type == SDL_CONTROLLERBUTTONUP) == ev_keyup;
		event.data1 = I_Translate_GameController(Event->cbutton.button);
		D_PostEvent(&event);
		break;

	case SDL_MOUSEBUTTONDOWN:
	case SDL_MOUSEBUTTONUP:
		if (!window_focused)
			break;

		event.type = (Event->type == SDL_MOUSEBUTTONUP) ? ev_mouseup : ev_mousedown;
		event.data1 =
			I_SDLtoDoomMouseState(SDL_GetMouseState(NULL, NULL));
		event.data2 = event.data3 = 0;

		D_PostEvent(&event);
		break;

	case SDL_MOUSEWHEEL:
		if (Event->wheel.y > 0) {
			event.type = ev_keydown;
			event.data1 = KEY_MWHEELUP;
			mwheeluptic = tic;
		}
		else if (Event->wheel.y < 0) {
			event.type = ev_keydown;
			event.data1 = KEY_MWHEELDOWN;
			mwheeldowntic = tic;
		}
		else
			break;

		event.data2 = event.data3 = 0;
		D_PostEvent(&event);
		break;

	case SDL_WINDOWEVENT:
		switch (Event->window.event) {
		case SDL_WINDOWEVENT_FOCUS_GAINED:
			window_focused = true;
			break;

		case SDL_WINDOWEVENT_FOCUS_LOST:
			window_focused = false;
			break;

		case SDL_WINDOWEVENT_ENTER:
			window_mouse = true;
			break;

		case SDL_WINDOWEVENT_LEAVE:
			window_mouse = false;
			break;

		default:
			break;
		}
		break;

	case SDL_QUIT:
		I_Quit();
		break;

	default:
		break;
	}

	if (s_controller)
	{
		event_t ev;

		x = SDL_GameControllerGetAxis(s_controller, SDL_CONTROLLER_AXIS_LEFTX);
		y = SDL_GameControllerGetAxis(s_controller, SDL_CONTROLLER_AXIS_LEFTY);

		s_clamp(x);
		s_clamp(y);

		ev.type = ev_gamepad;
		ev.data1 = GAMEPAD_LSTICK;
		ev.data2 = x;
		ev.data3 = -y;

		D_PostEvent(&ev);

		x = SDL_GameControllerGetAxis(s_controller, SDL_CONTROLLER_AXIS_RIGHTX);
		y = SDL_GameControllerGetAxis(s_controller, SDL_CONTROLLER_AXIS_RIGHTY);

		s_clamp(x);
		s_clamp(y);

		ev.type = ev_gamepad;
		ev.data1 = GAMEPAD_RSTICK;
		ev.data2 = x;
		ev.data3 = -y;
		D_PostEvent(&ev);

		static boolean old_ltrigger = false;
		static boolean old_rtrigger = false;

		int z = SDL_GameControllerGetAxis(s_controller, SDL_CONTROLLER_AXIS_TRIGGERLEFT);
		if (z >= 0x4000 && !old_ltrigger) {
			old_ltrigger = true;
			ev.type = ev_keydown;
			ev.data1 = GAMEPAD_LTRIGGER;
			D_PostEvent(&ev);
		}
		else if (z < 0x4000 && old_ltrigger) {
			old_ltrigger = false;
			ev.type = ev_keyup;
			ev.data1 = GAMEPAD_LTRIGGER;
			D_PostEvent(&ev);
		}

		z = SDL_GameControllerGetAxis(s_controller, SDL_CONTROLLER_AXIS_TRIGGERRIGHT);
		if (z >= 0x4000 && !old_rtrigger) {
			old_rtrigger = true;
			ev.type = ev_keydown;
			ev.data1 = GAMEPAD_RTRIGGER;
			D_PostEvent(&ev);
		}
		else if (z < 0x4000 && old_rtrigger) {
			old_rtrigger = false;
			ev.type = ev_keyup;
			ev.data1 = GAMEPAD_RTRIGGER;
			D_PostEvent(&ev);
		}
	}

	if (mwheeluptic && mwheeluptic + 1 < tic) {
		event.type = ev_keyup;
		event.data1 = KEY_MWHEELUP;
		D_PostEvent(&event);
		mwheeluptic = 0;
	}

	if (mwheeldowntic && mwheeldowntic + 1 < tic) {
		event.type = ev_keyup;
		event.data1 = KEY_MWHEELDOWN;
		D_PostEvent(&event);
		mwheeldowntic = 0;
	}
}

//
// I_ShutdownWait
//

int I_ShutdownWait(void) {
	static SDL_Event event;

	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_QUIT ||
			(event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)) {
			I_ShutdownVideo();
			return 1;
		}
	}

	return 0;
}

//
// I_StartTic
//

void I_StartTic(void) {
	SDL_Event Event;

	while (SDL_PollEvent(&Event)) {
	}

	I_InitInputs();
	I_ReadMouse();
}

//
// I_FinishUpdate
//

void I_FinishUpdate(void) {
	I_UpdateGrab();
	SDL_GL_SwapWindow(window);
	BusyDisk = false;
}

//
// I_InitInputs
//

void I_InitInputs(void) {
	SDL_PumpEvents();
	SDL_ShowCursor(m_menumouse.value < 1);
	I_MouseAccelChange();
}

//
// ISDL_RegisterCvars
//

void ISDL_RegisterKeyCvars(void) {
	CON_CvarRegister(&v_msensitivityx);
	CON_CvarRegister(&v_msensitivityy);
	CON_CvarRegister(&v_macceleration);
	CON_CvarRegister(&v_mlook);
	CON_CvarRegister(&v_mlookinvert);
	CON_CvarRegister(&v_yaxismove);
	CON_CvarRegister(&v_xaxismove);
}
