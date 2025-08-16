// Emacs style mode select   -*- C -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 1999-2000 Paul Brook
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
// DESCRIPTION: Key binding strings
//
//-----------------------------------------------------------------------------


#include <stdio.h>
#include <SDL3/SDL_stdinc.h>

#include "m_keys.h"
#include "doomdef.h"


typedef struct {
	int        code;
	char* name;
} keyinfo_t;

static keyinfo_t    Keys[] = {
	{KEY_RIGHTARROW,        "Right"},
	{KEY_LEFTARROW,         "Left"},
	{KEY_UPARROW,           "Up"},
	{KEY_DOWNARROW,         "Down"},
	{KEY_ESCAPE,            "Escape"},
	{KEY_ENTER,             "Enter"},
	{KEY_TAB,               "Tab"},
	{KEY_BACKSPACE,         "Backsp"},
	{KEY_PAUSE,             "Pause"},
	{KEY_SHIFT,             "Shift"},
	{KEY_ALT,               "Alt"},
	{KEY_CTRL,              "Ctrl"},
	{KEY_EQUALS,            "+"},
	{KEY_MINUS,             "-"},
	{KEY_ENTER,             "Enter"},
	{KEY_INSERT,            "Ins"},
	{KEY_DEL,               "Del"},
	{KEY_HOME,              "Home"},
	{KEY_END,               "End"},
	{KEY_PAGEUP,            "PgUp"},
	{KEY_PAGEDOWN,          "PgDn"},
	{';',                   ";"},
	{'\'',                  "'"},
	{'#',                   "#"},
	{'\\',                  "\\"},
	{',',                   ","},
	{'.',                   "."},
	{'/',                   "/"},
	{'[',                   "["},
	{']',                   "]"},
	{'*',                   "*"},
	{' ',                   "Space"},
	{KEY_F1,                "F1"},
	{KEY_F2,                "F2"},
	{KEY_F3,                "F3"},
	{KEY_F4,                "F4"},
	{KEY_F5,                "F5"},
	{KEY_F6,                "F6"},
	{KEY_F7,                "F7"},
	{KEY_F8,                "F8"},
	{KEY_F9,                "F9"},
	{KEY_F10,               "F10"},
	{KEY_F11,               "F11"},
	{KEY_F12,               "F12"},
	{KEY_KEYPADENTER,       "KeyPadEnter"},
	{KEY_KEYPADMULTIPLY,    "KeyPad*"},
	{KEY_KEYPADPLUS,        "KeyPad+"},
	{KEY_NUMLOCK,           "NumLock"},
	{KEY_KEYPADMINUS,       "KeyPad-"},
	{KEY_KEYPADPERIOD,      "KeyPad."},
	{KEY_KEYPADDIVIDE,      "KeyPad/"},
	{KEY_KEYPAD0,           "KeyPad0"},
	{KEY_KEYPAD1,           "KeyPad1"},
	{KEY_KEYPAD2,           "KeyPad2"},
	{KEY_KEYPAD3,           "KeyPad3"},
	{KEY_KEYPAD4,           "KeyPad4"},
	{KEY_KEYPAD5,           "KeyPad5"},
	{KEY_KEYPAD6,           "KeyPad6"},
	{KEY_KEYPAD7,           "KeyPad7"},
	{KEY_KEYPAD8,           "KeyPad8"},
	{KEY_KEYPAD9,           "KeyPad9"},
	{'0',                   "0"},
	{'1',                   "1"},
	{'2',                   "2"},
	{'3',                   "3"},
	{'4',                   "4"},
	{'5',                   "5"},
	{'6',                   "6"},
	{'7',                   "7"},
	{'8',                   "8"},
	{'9',                   "9"},
	{KEY_MWHEELUP,          "MouseWheelUp"},
	{KEY_MWHEELDOWN,        "MouseWheelDown"},

	{0,                 NULL}
};

//
// M_GetKeyName
//

int M_GetKeyName(char* buff, int key) {
	keyinfo_t* pkey;

	if (((key >= 'a') && (key <= 'z')) || ((key >= '0') && (key <= '9'))) {
		buff[0] = (char)SDL_toupper(key);
		buff[1] = 0;
		return true;
	}
	for (pkey = Keys; pkey->name; pkey++) {
		if (pkey->code == key) {
			dstrcpy(buff, pkey->name);
			return true;
		}
	}
	sprintf(buff, "Key%02x", key);
	return false;
}
