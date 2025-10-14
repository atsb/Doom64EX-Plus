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

#include <math.h>

#include "i_sdlinput.h"
#include "doomdef.h"
#include "doomstat.h"
#include "i_system.h"
#include "i_video.h"
#include "d_main.h"
#include "con_cvar.h"
#include "dgl.h"
#include "g_settings.h"

CVAR(v_msensitivityx, 5);
CVAR(v_msensitivityy, 5);
CVAR(v_macceleration, 0);
CVAR(v_mlookinvert, 0);
CVAR(v_yaxismove, 0);
CVAR(v_xaxismove, 0);
CVAR_EXTERNAL(m_menumouse);
CVAR_EXTERNAL(p_autoaim);

CVAR_CMD(v_mlook, 0) {
	if (cvar->value > 0) {
		if(!g_in_load_settings) {
			I_Printf("WARNING: mouse look: skies will not render properly with high pitch. Do not report.\n");
		}
		return;
	}

	// mlook is disabled

	// center player view, resetting pitch
	if (gamestate == GS_LEVEL) {
		players[0].mo->pitch = 0;
	}

	// force autoaim
	gameflags |= GF_ALLOWAUTOAIM;
};


float mouse_accelfactor;

int	UseJoystick;
int	DualMouse;

boolean	DigiJoy;
boolean	MouseMode;
boolean	window_mouse;

gamepad64_t gamepad64;

//
// SDL3 Gamepad
//

// Ugly shims to link the Gamepad with the keyboard / mouse controls
#define GAMEPAD_MENU_INITIAL_DELAY_TICS  12
#define GAMEPAD_MENU_REPEAT_TICS         4
#define GAMEPAD_MENU_STICK_THRESH        0.50f
#define GAMEPAD_LEFT_DEADZONE     0.18f
#define GAMEPAD_RIGHT_DEADZONE    0.20f
#define GAMEPAD_TRIGGER_THRESHOLD 0.30f
#define GAMEPAD_LOOK_BASE_SPEED   6.0f
#define GAMEPAD_KEY_MOVE_FWD      SDLK_W
#define GAMEPAD_KEY_MOVE_BACK     SDLK_S
#define GAMEPAD_KEY_MOVE_LEFT     SDLK_A
#define GAMEPAD_KEY_MOVE_RIGHT    SDLK_D
#define GAMEPAD_KEY_FIRE          KEY_CTRL
#define GAMEPAD_KEY_USE           SDLK_E
#define GAMEPAD_KEY_RUN			  KEY_SHIFT
#define GAMEPAD_KEY_AUTOMAP		  SDLK_TAB
#define GAMEPAD_KEY_NEXT_WEAPON   KEY_MWHEELUP
#define GAMEPAD_KEY_PREV_WEAPON   KEY_MWHEELDOWN
#define GAMEPAD_KEY_PAUSE         SDLK_ESCAPE
#define GAMEPAD_INNER_DZ_LEFT    0.18f
#define GAMEPAD_INNER_DZ_RIGHT   0.20f
#define GAMEPAD_OUTER_DZ         0.02f
#define GAMEPAD_ANTI_DZ          0.04f
#define GAMEPAD_EXPO_LEFT        1.20f
#define GAMEPAD_EXPO_RIGHT       1.60f
#define GAMEPAD_LOOK_SMOOTH_TC   0.060f
#define GAMEPAD_ADS_LTRIG        0.55f
#define GAMEPAD_ADS_SLOWDOWN     0.55f

extern void D_PostEvent(event_t*);
extern boolean menuactive;
extern gamestate_t gamestate;

static SDL_INLINE float I_GamepadClamp(float x) { return SDL_clamp(x, 0.f, 1.f); }

static void I_GamepadRadialLookSmoothing(float x, float y,
	float inner_dz, float outer_dz,
	float expo, float anti_dz,
	float* ox, float* oy)
{
	float r = SDL_sqrtf(x * x + y * y);
	if (r <= inner_dz) { *ox = 0.f; *oy = 0.f; return; }

	float nx = x / (r > 0.f ? r : 1.f);
	float ny = y / (r > 0.f ? r : 1.f);
	float span = 1.f - inner_dz - outer_dz;
	float t = I_GamepadClamp((r - inner_dz) / (span > 0.f ? span : 1.f));

	t = SDL_powf(t, expo);
	t = anti_dz + (1.f - anti_dz) * t;

	*ox = nx * t;
	*oy = ny * t;
}

static SDL_INLINE float I_GamepadLookSmoothing(float prev, float input, float dt, float tc) {
	float alpha = (tc > 0.f) ? (dt / (tc + dt)) : 1.f;
	alpha = SDL_clamp(alpha, 0.f, 1.f);
	return prev + alpha * (input - prev);
}

static SDL_INLINE void I_GamepadPostKeyEvent(int key, bool down) {
	event_t ev; ev.type = down ? ev_keydown : ev_keyup; ev.data1 = key; ev.data2 = ev.data3 = 0; ev.data4 = 0;
	D_PostEvent(&ev);
}

static SDL_INLINE void I_GamepadMenuEvent(bool now, bool* prev, int keycode, unsigned int* next_tic) {
	if (now && !*prev) {
		I_GamepadPostKeyEvent(keycode, true);
		*next_tic = gametic + GAMEPAD_MENU_INITIAL_DELAY_TICS;
	}
	else if (now && *prev && gametic >= *next_tic) {
		I_GamepadPostKeyEvent(keycode, true);
		*next_tic = gametic + GAMEPAD_MENU_REPEAT_TICS;
	}
	else if (!now && *prev) {
		I_GamepadPostKeyEvent(keycode, false);
	}
	*prev = now;
}

static SDL_INLINE void I_GamepadEdgeDetection(bool now, bool* prev, int keycode) {
	if (now != *prev) { I_GamepadPostKeyEvent(keycode, now); *prev = now; }
}

static SDL_INLINE void I_GamepadKeyRelease(void) {
	I_GamepadEdgeDetection(false, &gamepad64.player_forward, GAMEPAD_KEY_MOVE_FWD);
	I_GamepadEdgeDetection(false, &gamepad64.player_backwards, GAMEPAD_KEY_MOVE_BACK);
	I_GamepadEdgeDetection(false, &gamepad64.player_left, GAMEPAD_KEY_MOVE_LEFT);
	I_GamepadEdgeDetection(false, &gamepad64.player_right, GAMEPAD_KEY_MOVE_RIGHT);
	I_GamepadEdgeDetection(false, &gamepad64.player_fire, GAMEPAD_KEY_FIRE);
	I_GamepadEdgeDetection(false, &gamepad64.player_next_weapon, GAMEPAD_KEY_NEXT_WEAPON);
	I_GamepadEdgeDetection(false, &gamepad64.player_previous_weapon, GAMEPAD_KEY_PREV_WEAPON);
	I_GamepadEdgeDetection(false, &gamepad64.player_use, GAMEPAD_KEY_USE);
	I_GamepadEdgeDetection(false, &gamepad64.player_run, GAMEPAD_KEY_RUN);
	I_GamepadEdgeDetection(false, &gamepad64.player_automap, GAMEPAD_KEY_AUTOMAP);
}

static SDL_INLINE float I_GamepadAxisAlive(Sint16 v) {
	const float maxmag = (float)SDL_JOYSTICK_AXIS_MAX;
	float f = (float)v / maxmag;
	f = SDL_clamp(f, -1.f, 1.f);
	return f;
}
/*static SDL_INLINE float I_GamepadAxisDead(float v, float dz) {
	float a = fabsf(v);
	if (a <= dz) return 0.f;
	float sign = (v < 0.f) ? -1.f : 1.f;
	return sign * (a - dz) / (1.f - dz);
}*/
static SDL_INLINE void I_MouseRelease(int dx, int dy) {
	event_t ev; ev.type = ev_mouse;
	ev.data1 = 0;
	ev.data2 = dx * 32;
	ev.data3 = -dy * 32;
	ev.data4 = 0;
	D_PostEvent(&ev);
}

static void I_GamepadInit(void) {
	int n = 0; SDL_JoystickID* ids = SDL_GetGamepads(&n);
	if (ids && n > 0) {
		gamepad64.gamepad = SDL_OpenGamepad(ids[0]);
		if (gamepad64.gamepad) gamepad64.active_id = SDL_GetGamepadID(gamepad64.gamepad);
	}
	if (ids) SDL_free(ids);

	if (!gamepad64.gamepad) {
		int jn = 0; SDL_JoystickID* jids = SDL_GetJoysticks(&jn);
		if (jids && jn > 0) {
			if (SDL_IsGamepad(jids[0])) {
				gamepad64.gamepad = SDL_OpenGamepad(jids[0]);
				if (gamepad64.gamepad) gamepad64.active_id = SDL_GetGamepadID(gamepad64.gamepad);
			}
			if (!gamepad64.gamepad) {
				gamepad64.joy = SDL_OpenJoystick(jids[0]);
				if (gamepad64.joy) gamepad64.active_id = jids[0];
			}
		}
		if (jids) SDL_free(jids);
	}
}
static void I_GamepadClose(void) {
	if (gamepad64.gamepad) { SDL_CloseGamepad(gamepad64.gamepad); gamepad64.gamepad = NULL; }
	if (gamepad64.joy) { SDL_CloseJoystick(gamepad64.joy); gamepad64.joy = NULL; }
	gamepad64.active_id = 0;
}

static void I_GamepadInitOnce(void) {
	if (gamepad64.init) return;
	if (!SDL_WasInit(SDL_INIT_GAMEPAD)) SDL_InitSubSystem(SDL_INIT_GAMEPAD);
	I_GamepadInit();
	gamepad64.init = true;
}

static void I_GamepadHandleSDLEvent(const SDL_Event* e) {
	if (!gamepad64.init) return;
	switch (e->type) {
	case SDL_EVENT_GAMEPAD_ADDED:
		if (!gamepad64.gamepad && !gamepad64.joy) {
			gamepad64.gamepad = SDL_OpenGamepad(e->gdevice.which);
			if (gamepad64.gamepad) gamepad64.active_id = SDL_GetGamepadID(gamepad64.gamepad);
		}
		break;
	case SDL_EVENT_GAMEPAD_REMOVED:
		if (gamepad64.active_id == e->gdevice.which) I_GamepadClose();
		break;
	case SDL_EVENT_JOYSTICK_ADDED:
		if (!gamepad64.gamepad && !gamepad64.joy) {
			if (SDL_IsGamepad(e->jdevice.which)) {
				gamepad64.gamepad = SDL_OpenGamepad(e->jdevice.which);
				if (gamepad64.gamepad) gamepad64.active_id = SDL_GetGamepadID(gamepad64.gamepad);
			}
			else {
				gamepad64.joy = SDL_OpenJoystick(e->jdevice.which);
				if (gamepad64.joy) gamepad64.active_id = e->jdevice.which;
			}
		}
		break;
	case SDL_EVENT_JOYSTICK_REMOVED:
		if (gamepad64.active_id == e->jdevice.which) I_GamepadClose();
		break;
	default: break;
	}
}

static void I_GamepadUpdate(void) {
	if (!gamepad64.init) return;

	float lx_raw = 0.f, ly_raw = 0.f, rx_raw = 0.f, ry_raw = 0.f;
	float lt = 0.f, rt = 0.f;

	if (gamepad64.gamepad) {
		lx_raw = I_GamepadAxisAlive(SDL_GetGamepadAxis(gamepad64.gamepad, SDL_GAMEPAD_AXIS_LEFTX));
		ly_raw = I_GamepadAxisAlive(SDL_GetGamepadAxis(gamepad64.gamepad, SDL_GAMEPAD_AXIS_LEFTY));
		rx_raw = I_GamepadAxisAlive(SDL_GetGamepadAxis(gamepad64.gamepad, SDL_GAMEPAD_AXIS_RIGHTX));
		ry_raw = I_GamepadAxisAlive(SDL_GetGamepadAxis(gamepad64.gamepad, SDL_GAMEPAD_AXIS_RIGHTY));
		lt = (float)SDL_GetGamepadAxis(gamepad64.gamepad, SDL_GAMEPAD_AXIS_LEFT_TRIGGER) / (float)SDL_JOYSTICK_AXIS_MAX;
		rt = (float)SDL_GetGamepadAxis(gamepad64.gamepad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) / (float)SDL_JOYSTICK_AXIS_MAX;
	}
	else if (gamepad64.joy) {
		lx_raw = I_GamepadAxisAlive(SDL_GetJoystickAxis(gamepad64.joy, 0));
		ly_raw = I_GamepadAxisAlive(SDL_GetJoystickAxis(gamepad64.joy, 1));
		rx_raw = I_GamepadAxisAlive(SDL_GetJoystickAxis(gamepad64.joy, 2));
		ry_raw = I_GamepadAxisAlive(SDL_GetJoystickAxis(gamepad64.joy, 3));
	}
	float lx = 0.f, ly = 0.f, rx = 0.f, ry = 0.f;
	I_GamepadRadialLookSmoothing(lx_raw, ly_raw, GAMEPAD_INNER_DZ_LEFT, GAMEPAD_OUTER_DZ, GAMEPAD_EXPO_LEFT, GAMEPAD_ANTI_DZ, &lx, &ly);
	I_GamepadRadialLookSmoothing(rx_raw, ry_raw, GAMEPAD_INNER_DZ_RIGHT, GAMEPAD_OUTER_DZ, GAMEPAD_EXPO_RIGHT, GAMEPAD_ANTI_DZ, &rx, &ry);

	const bool in_menu = (menuactive || gamestate != GS_LEVEL);

	if (in_menu) {
		I_GamepadKeyRelease();

		bool d_up = false, d_down = false, d_left = false, d_right = false;
		if (gamepad64.gamepad) {
			d_up = SDL_GetGamepadButton(gamepad64.gamepad, SDL_GAMEPAD_BUTTON_DPAD_UP) != 0;
			d_down = SDL_GetGamepadButton(gamepad64.gamepad, SDL_GAMEPAD_BUTTON_DPAD_DOWN) != 0;
			d_left = SDL_GetGamepadButton(gamepad64.gamepad, SDL_GAMEPAD_BUTTON_DPAD_LEFT) != 0;
			d_right = SDL_GetGamepadButton(gamepad64.gamepad, SDL_GAMEPAD_BUTTON_DPAD_RIGHT) != 0;
		}
		else if (gamepad64.joy) {
			d_up = SDL_GetJoystickButton(gamepad64.joy, 10) != 0;
			d_down = SDL_GetJoystickButton(gamepad64.joy, 11) != 0;
			d_left = SDL_GetJoystickButton(gamepad64.joy, 12) != 0;
			d_right = SDL_GetJoystickButton(gamepad64.joy, 13) != 0;
		}
		bool s_up = false, s_down = false, s_left = false, s_right = false;
		if (!(d_up | d_down | d_left | d_right)) {
			s_up = (-ly) > GAMEPAD_MENU_STICK_THRESH;
			s_down = (-ly) < -GAMEPAD_MENU_STICK_THRESH;
			s_left = (lx) < -GAMEPAD_MENU_STICK_THRESH;
			s_right = (lx) > GAMEPAD_MENU_STICK_THRESH;
		}
		const bool mouse_up = d_up || s_up;
		const bool mouse_down = d_down || s_down;
		const bool mouse_left = d_left || s_left;
		const bool mouse_right = d_right || s_right;

		I_GamepadMenuEvent(mouse_up, &gamepad64.mouse_up, KEY_UPARROW, &gamepad64.right_arrow_key_up);
		I_GamepadMenuEvent(mouse_down, &gamepad64.mouse_down, KEY_DOWNARROW, &gamepad64.right_arrow_key_down);
		I_GamepadMenuEvent(mouse_left, &gamepad64.mouse_left, KEY_LEFTARROW, &gamepad64.right_arrow_key_left);
		I_GamepadMenuEvent(mouse_right, &gamepad64.mouse_right, KEY_RIGHTARROW, &gamepad64.right_arrow_key_right);

		bool a = false, b = false, startbtn = false, lb = false, rb = false;
		if (gamepad64.gamepad) {
			a = SDL_GetGamepadButton(gamepad64.gamepad, SDL_GAMEPAD_BUTTON_SOUTH) != 0;
			b = SDL_GetGamepadButton(gamepad64.gamepad, SDL_GAMEPAD_BUTTON_EAST) != 0;
			startbtn = SDL_GetGamepadButton(gamepad64.gamepad, SDL_GAMEPAD_BUTTON_START) != 0;
			lb = SDL_GetGamepadButton(gamepad64.gamepad, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER) != 0;
			rb = SDL_GetGamepadButton(gamepad64.gamepad, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER) != 0;
		}
		else if (gamepad64.joy) {
			a = SDL_GetJoystickButton(gamepad64.joy, 0) != 0;
			b = SDL_GetJoystickButton(gamepad64.joy, 1) != 0;
			startbtn = SDL_GetJoystickButton(gamepad64.joy, 9) != 0;
			lb = SDL_GetJoystickButton(gamepad64.joy, 4) != 0;
			rb = SDL_GetJoystickButton(gamepad64.joy, 5) != 0;
		}
		I_GamepadEdgeDetection(a, &gamepad64.mouse_accept, KEY_ENTER);
		I_GamepadEdgeDetection((b | startbtn), &gamepad64.mouse_back, KEY_ESCAPE);
		I_GamepadEdgeDetection(lb, &gamepad64.mouse_scroll_up, KEY_PAGEUP);
		I_GamepadEdgeDetection(rb, &gamepad64.mouse_scroll_down, KEY_PAGEDOWN);

		gamepad64.gamepad_look_dx = gamepad64.gamepad_look_dy = 0.f;
		return;
	}
	const float mth = 0.28f;
	const bool mv_fwd = (-ly) > mth;
	const bool mv_back = (-ly) < -mth;
	const bool mv_left = (lx) < -mth;
	const bool mv_right = (lx) > mth;

	I_GamepadEdgeDetection(mv_fwd, &gamepad64.player_forward, GAMEPAD_KEY_MOVE_FWD);
	I_GamepadEdgeDetection(mv_back, &gamepad64.player_backwards, GAMEPAD_KEY_MOVE_BACK);
	I_GamepadEdgeDetection(mv_left, &gamepad64.player_left, GAMEPAD_KEY_MOVE_LEFT);
	I_GamepadEdgeDetection(mv_right, &gamepad64.player_right, GAMEPAD_KEY_MOVE_RIGHT);

	float ads_scale = 1.f;
	if (gamepad64.gamepad) {
		if (lt >= GAMEPAD_ADS_LTRIG) ads_scale = GAMEPAD_ADS_SLOWDOWN;
	}
	else if (gamepad64.joy) {
		if (SDL_GetJoystickButton(gamepad64.joy, 6)) ads_scale = GAMEPAD_ADS_SLOWDOWN;
	}
	const float sensx = v_msensitivityx.value > 0 ? v_msensitivityx.value : 1.f;
	const float sensy = v_msensitivityy.value > 0 ? v_msensitivityy.value : 1.f;
	const float inv = (v_mlookinvert.value ? -1.f : 1.f);

	static float s_look_fx = 0.f, s_look_fy = 0.f;
	const float dt = (1.0f / 35.0f);
	s_look_fx = I_GamepadLookSmoothing(s_look_fx, rx, dt, GAMEPAD_LOOK_SMOOTH_TC);
	s_look_fy = I_GamepadLookSmoothing(s_look_fy, ry, dt, GAMEPAD_LOOK_SMOOTH_TC);

	const float dx = s_look_fx * GAMEPAD_LOOK_BASE_SPEED * sensx * ads_scale;
	const float dy = s_look_fy * GAMEPAD_LOOK_BASE_SPEED * sensy * ads_scale * inv;

	gamepad64.gamepad_look_dx += dx; gamepad64.gamepad_look_dy += dy;
	const int sdx = (int)gamepad64.gamepad_look_dx;
	const int sdy = (int)gamepad64.gamepad_look_dy;
	if (sdx != 0 || sdy != 0) {
		I_MouseRelease(sdx, sdy);
		gamepad64.gamepad_look_dx -= sdx; gamepad64.gamepad_look_dy -= sdy;
	}
	bool fire = (rt >= GAMEPAD_TRIGGER_THRESHOLD);
	bool alt = (lt >= GAMEPAD_TRIGGER_THRESHOLD);
	bool nextw = false, prevw = false, use = false, pausebtn = false, run = false, automap = false;

	if (gamepad64.gamepad) {
		nextw = SDL_GetGamepadButton(gamepad64.gamepad, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER) != 0;
		prevw = SDL_GetGamepadButton(gamepad64.gamepad, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER) != 0;
		use = SDL_GetGamepadButton(gamepad64.gamepad, SDL_GAMEPAD_BUTTON_SOUTH) != 0;
		run = SDL_GetGamepadButton(gamepad64.gamepad, SDL_GAMEPAD_BUTTON_WEST) != 0;
		automap = SDL_GetGamepadButton(gamepad64.gamepad, SDL_GAMEPAD_BUTTON_NORTH) != 0;
		pausebtn = SDL_GetGamepadButton(gamepad64.gamepad, SDL_GAMEPAD_BUTTON_START) != 0;
	}
	else if (gamepad64.joy) {
		nextw = SDL_GetJoystickButton(gamepad64.joy, 5) != 0;
		prevw = SDL_GetJoystickButton(gamepad64.joy, 4) != 0;
		use = SDL_GetJoystickButton(gamepad64.joy, 2) != 0;
		run = SDL_GetJoystickButton(gamepad64.joy, 6) != 0;
		automap = SDL_GetJoystickButton(gamepad64.joy, 7) != 0;
		pausebtn = SDL_GetJoystickButton(gamepad64.joy, 9) != 0;
		if (!fire) fire = SDL_GetJoystickButton(gamepad64.joy, 7) != 0;
		if (!alt)  alt = SDL_GetJoystickButton(gamepad64.joy, 6) != 0;
	}
	I_GamepadEdgeDetection(fire, &gamepad64.player_fire, GAMEPAD_KEY_FIRE);
	I_GamepadEdgeDetection(nextw, &gamepad64.player_next_weapon, GAMEPAD_KEY_NEXT_WEAPON);
	I_GamepadEdgeDetection(prevw, &gamepad64.player_previous_weapon, GAMEPAD_KEY_PREV_WEAPON);
	I_GamepadEdgeDetection(use, &gamepad64.player_use, GAMEPAD_KEY_USE);
	I_GamepadEdgeDetection(pausebtn, &gamepad64.player_pause, GAMEPAD_KEY_PAUSE);
	I_GamepadEdgeDetection(run, &gamepad64.player_run, GAMEPAD_KEY_RUN);
	I_GamepadEdgeDetection(automap, &gamepad64.player_automap, GAMEPAD_KEY_AUTOMAP);
}

//
// I_TranslateKey
//

// Modernised, it was really needed!
static int I_TranslateKey(SDL_KeyboardEvent *key_event)
{
	static struct { int sdl; int eng; } map[] = {
		{ SDLK_LEFT,        KEY_LEFTARROW },
		{ SDLK_RIGHT,       KEY_RIGHTARROW },
		{ SDLK_UP,          KEY_UPARROW },
		{ SDLK_DOWN,        KEY_DOWNARROW },
		{ SDLK_ESCAPE,      KEY_ESCAPE },
		{ SDLK_RETURN,      KEY_ENTER },
		{ SDLK_TAB,         KEY_TAB },
		{ SDLK_BACKSPACE,   KEY_BACKSPACE },
		{ SDLK_DELETE,      KEY_DEL },
		{ SDLK_INSERT,      KEY_INSERT },
		{ SDLK_HOME,        KEY_HOME },
		{ SDLK_END,         KEY_END },
		{ SDLK_PAGEUP,      KEY_PAGEUP },
		{ SDLK_PAGEDOWN,    KEY_PAGEDOWN },
		{ SDLK_PAUSE,       KEY_PAUSE },
		{ SDLK_LSHIFT,      KEY_RSHIFT },
		{ SDLK_RSHIFT,      KEY_RSHIFT },
		{ SDLK_LCTRL,       KEY_RCTRL },
		{ SDLK_RCTRL,       KEY_RCTRL },
		{ SDLK_LALT,        KEY_RALT },
		{ SDLK_RALT,        KEY_RALT },
		{ SDLK_EQUALS,      KEY_EQUALS },
		{ SDLK_MINUS,       KEY_MINUS },
		{ SDLK_SPACE,       KEY_SPACEBAR },
		{ SDLK_F1,          KEY_F1 },
		{ SDLK_F2,			KEY_F2  },
		{ SDLK_F3,			KEY_F3  },
		{ SDLK_F4,			KEY_F4  },
		{ SDLK_F5,          KEY_F5 },
		{ SDLK_F6,			KEY_F6  },
		{ SDLK_F7,			KEY_F7  },
		{ SDLK_F8,			KEY_F8  },
		{ SDLK_F9,          KEY_F9 },
		{ SDLK_F10,			KEY_F10 },
		{ SDLK_F11,			KEY_F11 },
		{ SDLK_F12,			KEY_F12 },
		{ SDLK_KP_0,        KEY_KEYPAD0 },
		{ SDLK_KP_1,        KEY_KEYPAD1 },
		{ SDLK_KP_2,        KEY_KEYPAD2 },
		{ SDLK_KP_3,        KEY_KEYPAD3 },
		{ SDLK_KP_4,        KEY_KEYPAD4 },
		{ SDLK_KP_5,        KEY_KEYPAD5 },
		{ SDLK_KP_6,        KEY_KEYPAD6 },
		{ SDLK_KP_7,        KEY_KEYPAD7 },
		{ SDLK_KP_8,        KEY_KEYPAD8 },
		{ SDLK_KP_9,        KEY_KEYPAD9 },
		{ SDLK_KP_ENTER,    KEY_KEYPADENTER },
		{ SDLK_KP_MULTIPLY, KEY_KEYPADMULTIPLY },
		{ SDLK_KP_PLUS,     KEY_KEYPADPLUS },
		{ SDLK_KP_MINUS,    KEY_KEYPADMINUS },
		{ SDLK_KP_DIVIDE,   KEY_KEYPADDIVIDE },
		{ SDLK_KP_PERIOD,   KEY_KEYPADPERIOD },
	};

	const int key = key_event->key;
		
	// key reserved for the console, independent of location on keyboard
	// Located in the top left corner (on both ANSI and ISO keyboards).
	if(key_event->scancode == SDL_SCANCODE_GRAVE) {
		return KEY_CONSOLE;
	}
	
	for (size_t i = 0; i < sizeof(map) / sizeof(map[0]); ++i) {
		if (key == map[i].sdl) return map[i].eng;
	}
	
	if (key >= 32 && key < 127) {
		return key;
	}
	
	return 0;
}

//
// I_SDLtoDoomMouseState
//

static int I_SDLtoDoomMouseState(Uint8 buttonstate) {
	return 0
		| (buttonstate & SDL_BUTTON_LMASK ? 1 : 0)
		| (buttonstate & SDL_BUTTON_MMASK ? 2 : 0)
		| (buttonstate & SDL_BUTTON_RMASK ? 4 : 0)
		| (buttonstate & SDL_BUTTON_X1MASK ? 8 : 0)
		| (buttonstate & SDL_BUTTON_X2MASK ? 16 : 0);
}

//
// I_ReadMouse
//

void I_ReadMouse(void) {
	float x, y;
	Uint8 btn;
	event_t ev;
	static Uint8 lastmbtn = 0;

	SDL_GetRelativeMouseState(&x, &y);
	btn = SDL_GetMouseState(&mouse_x, &mouse_y);

	if (x != 0 || y != 0 || btn || (lastmbtn != btn)) {
		ev.type = ev_mouse;
		ev.data1 = I_SDLtoDoomMouseState(btn);
		ev.data2 = x * 32.0;
		ev.data3 = -y * 32.0;
		ev.data4 = 0;
		D_PostEvent(&ev);
	}

	lastmbtn = btn;
}

void I_CenterMouse(void) {
	SDL_WarpMouseInWindow(window, (unsigned short)(video_width / 2), (unsigned short)(video_height / 2));
	SDL_PumpEvents();
	SDL_GetRelativeMouseState(NULL, NULL);
}


void I_SetMousePos(float x, float y) {
	SDL_WarpMouseInWindow(window, (unsigned short)x, (unsigned short)y);
	SDL_PumpEvents();
	SDL_GetMouseState(&mouse_x, &mouse_y); // refresh new location
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

float I_MouseAccel(float val) {
	if (!v_macceleration.value) {
		return val;
	}

	if (val < 0) {
		return -I_MouseAccel(-val);
	}

	return (float)(pow((double)val, (double)mouse_accelfactor));
}

//
// I_UpdateGrab
//

boolean I_UpdateGrab(void) {

	static boolean currently_grabbed = false;
	boolean grab;

	grab = !menuactive
		&& (gamestate == GS_LEVEL)
		&& !demoplayback;

	/*
		Don't grab the keyboard (SDL_SetWindowKeyboardGrab) because:
			- we don't need it
			- it mess up Alt-Tab (only works when not grabbed). And Tab switches the automap modes
			- on Linux KDE (Plasma) it prevents global shortcuts to work (brightness, sound, ...)
			- finally, the SDL doc does not recommend it: "Normal games should not use keyboard grab"
	*/

	if (grab && !currently_grabbed) {
		SDL_SetWindowRelativeMouseMode(window, 1); // this grabs the mouse and hide cursor
		SDL_SetWindowMouseGrab(window, 1);
	}

	if (!grab && currently_grabbed) {
		SDL_SetWindowRelativeMouseMode(window, 0);
		SDL_SetWindowMouseGrab(window, 0);
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

	I_GamepadHandleSDLEvent(Event);
	
	switch (Event->type) {
	case SDL_EVENT_KEY_DOWN:
		if (Event->key.repeat) {
			break;
		}
		event.type = ev_keydown;
		event.data1 = I_TranslateKey(&Event->key);
		if(event.data1) {
			D_PostEvent(&event);
		}
		break;

	case SDL_EVENT_KEY_UP:
		event.type = ev_keyup;
		event.data1 = I_TranslateKey(&Event->key);
		if(event.data1) {
			D_PostEvent(&event);
		}
		break;

	case SDL_EVENT_MOUSE_BUTTON_DOWN:
	case SDL_EVENT_MOUSE_BUTTON_UP:
		if (!window_focused)
			break;

		event.type = (Event->type == SDL_EVENT_MOUSE_BUTTON_UP) ? ev_mouseup : ev_mousedown;
		event.data1 =
			I_SDLtoDoomMouseState(SDL_GetMouseState(NULL, NULL));
		event.data2 = event.data3 = 0;

		D_PostEvent(&event);
		break;

	case SDL_EVENT_MOUSE_WHEEL:
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

	case SDL_EVENT_WINDOW_FOCUS_GAINED:
		window_focused = true;
		break;

	case SDL_EVENT_WINDOW_FOCUS_LOST:
		window_focused = false;
		break;

	case SDL_EVENT_WINDOW_MOUSE_ENTER:
		window_mouse = true;
		break;

	case SDL_EVENT_WINDOW_MOUSE_LEAVE:
		window_mouse = false;
		break;

	case SDL_EVENT_QUIT:
		I_Quit();
		break;

	default:
		break;
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
		if (event.type == SDL_EVENT_QUIT ||
			(event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE)) {
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
		I_GetEvent(&Event);
	}

	I_InitInputs();
	I_ReadMouse();
	I_GamepadUpdate();
}

static float GetDisplayRefreshRate(void) {
	SDL_DisplayID displayid = SDL_GetDisplayForWindow(window);
	if (displayid) {
		const SDL_DisplayMode* mode = SDL_GetCurrentDisplayMode(displayid);
		if (mode && mode->refresh_rate > 0) {
			return mode->refresh_rate;
		}
	}
	return 60.0f; // not happy, return to 60
}

//
// I_FinishUpdate
//

void I_FinishUpdate(void) {
	I_UpdateGrab();
	SDL_GL_SwapWindow(window);
	dglFinish();
	BusyDisk = false;
}

//
// I_InitInputs
//

void I_InitInputs(void) {
	SDL_PumpEvents();
	I_MouseAccelChange();
	I_GamepadInitOnce();
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
