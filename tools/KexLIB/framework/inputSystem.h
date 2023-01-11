// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
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

#ifndef __INPUT_SYSTEM_H__
#define __INPUT_SYSTEM_H__

// Input event types.
typedef enum {
    ev_keydown,
    ev_keyup,
    ev_mouse,
    ev_mousedown,
    ev_mouseup,
    ev_mousewheel,
    ev_gamepad
} inputType_t;

// Event structure.
typedef struct {
    inputType_t type;
    int         data1;  // keys / mouse/joystick buttons
    int         data2;  // mouse/joystick x move
    int         data3;  // mouse/joystick y move
    int         data4;  // misc data
} inputEvent_t;

// the following is based directly off of SDL_scancode.h and SDL_keycode.h
#define KKEY_SCANCODE_MASK (1<<30)
#define KKEY_SCANCODE_TO_KEYCODE(x)  (x | KKEY_SCANCODE_MASK)

typedef enum {
    SKEY_UNDEFINED = 0,
    SKEY_SCANCODE_A = 4,
    SKEY_SCANCODE_B = 5,
    SKEY_SCANCODE_C = 6,
    SKEY_SCANCODE_D = 7,
    SKEY_SCANCODE_E = 8,
    SKEY_SCANCODE_F = 9,
    SKEY_SCANCODE_G = 10,
    SKEY_SCANCODE_H = 11,
    SKEY_SCANCODE_I = 12,
    SKEY_SCANCODE_J = 13,
    SKEY_SCANCODE_K = 14,
    SKEY_SCANCODE_L = 15,
    SKEY_SCANCODE_M = 16,
    SKEY_SCANCODE_N = 17,
    SKEY_SCANCODE_O = 18,
    SKEY_SCANCODE_P = 19,
    SKEY_SCANCODE_Q = 20,
    SKEY_SCANCODE_R = 21,
    SKEY_SCANCODE_S = 22,
    SKEY_SCANCODE_T = 23,
    SKEY_SCANCODE_U = 24,
    SKEY_SCANCODE_V = 25,
    SKEY_SCANCODE_W = 26,
    SKEY_SCANCODE_X = 27,
    SKEY_SCANCODE_Y = 28,
    SKEY_SCANCODE_Z = 29,

    SKEY_SCANCODE_1 = 30,
    SKEY_SCANCODE_2 = 31,
    SKEY_SCANCODE_3 = 32,
    SKEY_SCANCODE_4 = 33,
    SKEY_SCANCODE_5 = 34,
    SKEY_SCANCODE_6 = 35,
    SKEY_SCANCODE_7 = 36,
    SKEY_SCANCODE_8 = 37,
    SKEY_SCANCODE_9 = 38,
    SKEY_SCANCODE_0 = 39,

    SKEY_SCANCODE_RETURN = 40,
    SKEY_SCANCODE_ESCAPE = 41,
    SKEY_SCANCODE_BACKSPACE = 42,
    SKEY_SCANCODE_TAB = 43,
    SKEY_SCANCODE_SPACE = 44,

    SKEY_SCANCODE_MINUS = 45,
    SKEY_SCANCODE_EQUALS = 46,
    SKEY_SCANCODE_LEFTBRACKET = 47,
    SKEY_SCANCODE_RIGHTBRACKET = 48,
    SKEY_SCANCODE_BACKSLASH = 49,
    SKEY_SCANCODE_NONUSHASH = 50,
    SKEY_SCANCODE_SEMICOLON = 51,
    SKEY_SCANCODE_APOSTROPHE = 52,
    SKEY_SCANCODE_GRAVE = 53,
    SKEY_SCANCODE_COMMA = 54,
    SKEY_SCANCODE_PERIOD = 55,
    SKEY_SCANCODE_SLASH = 56,
    SKEY_SCANCODE_CAPSLOCK = 57,
    SKEY_SCANCODE_F1 = 58,
    SKEY_SCANCODE_F2 = 59,
    SKEY_SCANCODE_F3 = 60,
    SKEY_SCANCODE_F4 = 61,
    SKEY_SCANCODE_F5 = 62,
    SKEY_SCANCODE_F6 = 63,
    SKEY_SCANCODE_F7 = 64,
    SKEY_SCANCODE_F8 = 65,
    SKEY_SCANCODE_F9 = 66,
    SKEY_SCANCODE_F10 = 67,
    SKEY_SCANCODE_F11 = 68,
    SKEY_SCANCODE_F12 = 69,
    SKEY_SCANCODE_PRINTSCREEN = 70,
    SKEY_SCANCODE_SCROLLLOCK = 71,
    SKEY_SCANCODE_PAUSE = 72,
    SKEY_SCANCODE_INSERT = 73,
    SKEY_SCANCODE_HOME = 74,
    SKEY_SCANCODE_PAGEUP = 75,
    SKEY_SCANCODE_DELETE = 76,
    SKEY_SCANCODE_END = 77,
    SKEY_SCANCODE_PAGEDOWN = 78,
    SKEY_SCANCODE_RIGHT = 79,
    SKEY_SCANCODE_LEFT = 80,
    SKEY_SCANCODE_DOWN = 81,
    SKEY_SCANCODE_UP = 82,
    SKEY_SCANCODE_NUMLOCKCLEAR = 83,
    SKEY_SCANCODE_KP_DIVIDE = 84,
    SKEY_SCANCODE_KP_MULTIPLY = 85,
    SKEY_SCANCODE_KP_MINUS = 86,
    SKEY_SCANCODE_KP_PLUS = 87,
    SKEY_SCANCODE_KP_ENTER = 88,
    SKEY_SCANCODE_KP_1 = 89,
    SKEY_SCANCODE_KP_2 = 90,
    SKEY_SCANCODE_KP_3 = 91,
    SKEY_SCANCODE_KP_4 = 92,
    SKEY_SCANCODE_KP_5 = 93,
    SKEY_SCANCODE_KP_6 = 94,
    SKEY_SCANCODE_KP_7 = 95,
    SKEY_SCANCODE_KP_8 = 96,
    SKEY_SCANCODE_KP_9 = 97,
    SKEY_SCANCODE_KP_0 = 98,
    SKEY_SCANCODE_KP_PERIOD = 99,
    SKEY_SCANCODE_LCTRL = 224,
    SKEY_SCANCODE_LSHIFT = 225,
    SKEY_SCANCODE_LALT = 226,
    SKEY_SCANCODE_RCTRL = 228,
    SKEY_SCANCODE_RSHIFT = 229,
    SKEY_SCANCODE_RALT = 230,

    SKEY_NUMSCANKEYS
} scanKey_t;

typedef enum {
    KKEY_UNDEFINED = 0,
    KKEY_RETURN = '\r',
    KKEY_ESCAPE = '\033',
    KKEY_BACKSPACE = '\b',
    KKEY_TAB = '\t',
    KKEY_SPACE = ' ',
    KKEY_EXCLAIM = '!',
    KKEY_QUOTEDBL = '"',
    KKEY_HASH = '#',
    KKEY_PERCENT = '%',
    KKEY_DOLLAR = '$',
    KKEY_AMPERSAND = '&',
    KKEY_QUOTE = '\'',
    KKEY_LEFTPAREN = '(',
    KKEY_RIGHTPAREN = ')',
    KKEY_ASTERISK = '*',
    KKEY_PLUS = '+',
    KKEY_COMMA = ',',
    KKEY_MINUS = '-',
    KKEY_PERIOD = '.',
    KKEY_SLASH = '/',
    KKEY_0 = '0',
    KKEY_1 = '1',
    KKEY_2 = '2',
    KKEY_3 = '3',
    KKEY_4 = '4',
    KKEY_5 = '5',
    KKEY_6 = '6',
    KKEY_7 = '7',
    KKEY_8 = '8',
    KKEY_9 = '9',
    KKEY_COLON = ':',
    KKEY_SEMICOLON = ';',
    KKEY_LESS = '<',
    KKEY_EQUALS = '=',
    KKEY_GREATER = '>',
    KKEY_QUESTION = '?',
    KKEY_AT = '@',
    KKEY_LEFTBRACKET = '[',
    KKEY_BACKSLASH = '\\',
    KKEY_RIGHTBRACKET = ']',
    KKEY_CARET = '^',
    KKEY_UNDERSCORE = '_',
    KKEY_BACKQUOTE = '`',
    KKEY_a = 'a',
    KKEY_b = 'b',
    KKEY_c = 'c',
    KKEY_d = 'd',
    KKEY_e = 'e',
    KKEY_f = 'f',
    KKEY_g = 'g',
    KKEY_h = 'h',
    KKEY_i = 'i',
    KKEY_j = 'j',
    KKEY_k = 'k',
    KKEY_l = 'l',
    KKEY_m = 'm',
    KKEY_n = 'n',
    KKEY_o = 'o',
    KKEY_p = 'p',
    KKEY_q = 'q',
    KKEY_r = 'r',
    KKEY_s = 's',
    KKEY_t = 't',
    KKEY_u = 'u',
    KKEY_v = 'v',
    KKEY_w = 'w',
    KKEY_x = 'x',
    KKEY_y = 'y',
    KKEY_z = 'z',

    KKEY_CAPSLOCK = KKEY_SCANCODE_TO_KEYCODE(SKEY_SCANCODE_CAPSLOCK),

    KKEY_F1 = KKEY_SCANCODE_TO_KEYCODE(SKEY_SCANCODE_F1),
    KKEY_F2 = KKEY_SCANCODE_TO_KEYCODE(SKEY_SCANCODE_F2),
    KKEY_F3 = KKEY_SCANCODE_TO_KEYCODE(SKEY_SCANCODE_F3),
    KKEY_F4 = KKEY_SCANCODE_TO_KEYCODE(SKEY_SCANCODE_F4),
    KKEY_F5 = KKEY_SCANCODE_TO_KEYCODE(SKEY_SCANCODE_F5),
    KKEY_F6 = KKEY_SCANCODE_TO_KEYCODE(SKEY_SCANCODE_F6),
    KKEY_F7 = KKEY_SCANCODE_TO_KEYCODE(SKEY_SCANCODE_F7),
    KKEY_F8 = KKEY_SCANCODE_TO_KEYCODE(SKEY_SCANCODE_F8),
    KKEY_F9 = KKEY_SCANCODE_TO_KEYCODE(SKEY_SCANCODE_F9),
    KKEY_F10 = KKEY_SCANCODE_TO_KEYCODE(SKEY_SCANCODE_F10),
    KKEY_F11 = KKEY_SCANCODE_TO_KEYCODE(SKEY_SCANCODE_F11),
    KKEY_F12 = KKEY_SCANCODE_TO_KEYCODE(SKEY_SCANCODE_F12),

    KKEY_PRINTSCREEN = KKEY_SCANCODE_TO_KEYCODE(SKEY_SCANCODE_PRINTSCREEN),
    KKEY_SCROLLLOCK = KKEY_SCANCODE_TO_KEYCODE(SKEY_SCANCODE_SCROLLLOCK),
    KKEY_PAUSE = KKEY_SCANCODE_TO_KEYCODE(SKEY_SCANCODE_PAUSE),
    KKEY_INSERT = KKEY_SCANCODE_TO_KEYCODE(SKEY_SCANCODE_INSERT),
    KKEY_HOME = KKEY_SCANCODE_TO_KEYCODE(SKEY_SCANCODE_HOME),
    KKEY_PAGEUP = KKEY_SCANCODE_TO_KEYCODE(SKEY_SCANCODE_PAGEUP),
    KKEY_DELETE = '\177',
    KKEY_END = KKEY_SCANCODE_TO_KEYCODE(SKEY_SCANCODE_END),
    KKEY_PAGEDOWN = KKEY_SCANCODE_TO_KEYCODE(SKEY_SCANCODE_PAGEDOWN),
    KKEY_RIGHT = KKEY_SCANCODE_TO_KEYCODE(SKEY_SCANCODE_RIGHT),
    KKEY_LEFT = KKEY_SCANCODE_TO_KEYCODE(SKEY_SCANCODE_LEFT),
    KKEY_DOWN = KKEY_SCANCODE_TO_KEYCODE(SKEY_SCANCODE_DOWN),
    KKEY_UP = KKEY_SCANCODE_TO_KEYCODE(SKEY_SCANCODE_UP),

    KKEY_NUMLOCKCLEAR = KKEY_SCANCODE_TO_KEYCODE(SKEY_SCANCODE_NUMLOCKCLEAR),
    KKEY_KP_DIVIDE = KKEY_SCANCODE_TO_KEYCODE(SKEY_SCANCODE_KP_DIVIDE),
    KKEY_KP_MULTIPLY = KKEY_SCANCODE_TO_KEYCODE(SKEY_SCANCODE_KP_MULTIPLY),
    KKEY_KP_MINUS = KKEY_SCANCODE_TO_KEYCODE(SKEY_SCANCODE_KP_MINUS),
    KKEY_KP_PLUS = KKEY_SCANCODE_TO_KEYCODE(SKEY_SCANCODE_KP_PLUS),
    KKEY_KP_ENTER = KKEY_SCANCODE_TO_KEYCODE(SKEY_SCANCODE_KP_ENTER),
    KKEY_KP_1 = KKEY_SCANCODE_TO_KEYCODE(SKEY_SCANCODE_KP_1),
    KKEY_KP_2 = KKEY_SCANCODE_TO_KEYCODE(SKEY_SCANCODE_KP_2),
    KKEY_KP_3 = KKEY_SCANCODE_TO_KEYCODE(SKEY_SCANCODE_KP_3),
    KKEY_KP_4 = KKEY_SCANCODE_TO_KEYCODE(SKEY_SCANCODE_KP_4),
    KKEY_KP_5 = KKEY_SCANCODE_TO_KEYCODE(SKEY_SCANCODE_KP_5),
    KKEY_KP_6 = KKEY_SCANCODE_TO_KEYCODE(SKEY_SCANCODE_KP_6),
    KKEY_KP_7 = KKEY_SCANCODE_TO_KEYCODE(SKEY_SCANCODE_KP_7),
    KKEY_KP_8 = KKEY_SCANCODE_TO_KEYCODE(SKEY_SCANCODE_KP_8),
    KKEY_KP_9 = KKEY_SCANCODE_TO_KEYCODE(SKEY_SCANCODE_KP_9),
    KKEY_KP_0 = KKEY_SCANCODE_TO_KEYCODE(SKEY_SCANCODE_KP_0),
    KKEY_KP_PERIOD = KKEY_SCANCODE_TO_KEYCODE(SKEY_SCANCODE_KP_PERIOD),

    KKEY_LCTRL = KKEY_SCANCODE_TO_KEYCODE(SKEY_SCANCODE_LCTRL),
    KKEY_LSHIFT = KKEY_SCANCODE_TO_KEYCODE(SKEY_SCANCODE_LSHIFT),
    KKEY_LALT = KKEY_SCANCODE_TO_KEYCODE(SKEY_SCANCODE_LALT),
    KKEY_RCTRL = KKEY_SCANCODE_TO_KEYCODE(SKEY_SCANCODE_RCTRL),
    KKEY_RSHIFT = KKEY_SCANCODE_TO_KEYCODE(SKEY_SCANCODE_RSHIFT),
    KKEY_RALT = KKEY_SCANCODE_TO_KEYCODE(SKEY_SCANCODE_RALT)
} inputKey_t;

// from SDL_mouse.h
#define KM_BUTTON(x)            (1 << ((x)-1))
#define KM_BUTTON_LEFT          1
#define KM_BUTTON_MIDDLE        2
#define KM_BUTTON_RIGHT         3
#define KM_BUTTON_SCROLL_UP     4
#define KM_BUTTON_SCROLL_DOWN   5

class kexInputSystem {
public:
                            kexInputSystem(void);

    virtual void            Init(void) = 0;
    virtual void            PumpInputEvents(void) = 0;
    virtual void            PollInput(void) = 0;

    bool                    IsShiftDown(int c) const;
    bool                    IsCtrlDown(int c) const;
    bool                    IsAltDown(int c) const;

    virtual unsigned int    MouseGetState(int *x, int *y) = 0;
    virtual unsigned int    MouseGetRelativeState(int *x, int *y) = 0;
    virtual void            MouseCenter(void) = 0;
    virtual void            MouseRead(void) = 0;
    virtual void            MouseMove(const int x, const int y) = 0;
    virtual void            MouseActivate(const bool bToggle) = 0;
    virtual void            MouseUpdateGrab(void) = 0;
    virtual void            MouseUpdateFocus(void) = 0;
    
    void                    SetEnabled(bool enable) { bEnabled = enable; }
    const int               Mouse_X(void) const { return mouse_x; }
    const int               Mouse_Y(void) const { return mouse_y; }

protected:
    bool                    bEnabled;
    bool                    bMouseGrabbed;

    int                     mouse_x;
    int                     mouse_y;
};

//
// kexInputSystem::kexInputSystem
//

d_inline kexInputSystem::kexInputSystem(void) {
    bMouseGrabbed   = false;
    bEnabled        = true;
}

//
// kexInputSystem::IsShiftDown
//

d_inline bool kexInputSystem::IsShiftDown(int c) const {
    return(c == KKEY_LSHIFT || c == KKEY_RSHIFT);
}

//
// kexInputSystem::IsCtrlDown
//

d_inline bool kexInputSystem::IsCtrlDown(int c) const {
    return(c == KKEY_LCTRL || c == KKEY_RCTRL);
}

//
// kexInputSystem::IsAltDown
//

d_inline bool kexInputSystem::IsAltDown(int c) const {
    return(c == KKEY_LALT || c == KKEY_RALT);
}

#endif
