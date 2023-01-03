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

#ifndef __DOOMDEF__
#define __DOOMDEF__

#ifdef _WIN32
#ifdef _XBOX
#include <xtl.h>
#else
#include <windows.h>
#endif
#ifdef _XBOX
#include <wctype.h>
#else
#include <wtypes.h>//only for GUID type
#endif
#endif // _WIN32

#include <stdio.h>

#include "doomtype.h"
#include "tables.h"
#include "i_w3swrapper.h"

// #define macros to provide functions missing in Windows.
// Outside Windows, we use strings.h for str[n]casecmp.

//
// The packed attribute forces structures to be packed into the minimum
// space necessary.  If this is not done, the compiler may align structure
// fields differently to optimise memory access, inflating the overall
// structure size.  It is important to use the packed attribute on certain
// structures where alignment is important, particularly data read/written
// to disk.
//

#ifndef __GNUC__
#define __attribute__(x)
#endif

#ifdef __GNUC__
#define PACKEDATTR __attribute__((packed))
#else
#define PACKEDATTR
#endif

//
// Global parameters/defines.
//

#define SCREENWIDTH     320
#define SCREENHEIGHT    240

#define MAX_MESSAGE_SIZE 1024

// If rangecheck is undefined,
// most parameter validation debugging code will not be compiled
//#define RANGECHECK

//villsa
#define D_RGBA(r,g,b,a) ((rcolor)((((a)&0xff)<<24)|(((b)&0xff)<<16)|(((g)&0xff)<<8)|((r)&0xff)))

// basic color definitions

#define WHITE               0xFFFFFFFF
#define RED                 0xFF0000FF
#define BLUE                0xFFFF0000
#define AQUA                0xFFFFFF00
#define YELLOW              0xFF00FFFF
#define GREEN               0xFF00FF00
#define REDALPHA(x)         (x<<24|0x0000FF)
#define WHITEALPHA(x)       (x<<24|0xFFFFFF)

// The maximum number of players, multiplayer/networking.
// remember to add settings for extra skins if increase:)
#define MAXPLAYERS      4

// State updates, number of tics / second.
#define TICRATE         30

// The current state of the game: whether we are
// playing, gazing at the intermission screen,
// the game final animation, or a demo.
typedef enum {
	GS_NONE,
	GS_LEVEL,
	GS_SKIPPABLE
} gamestate_t;

//
// Difficulty/skill settings/filters.
//

// Skill flags.
enum
{
	MTF_EASY = 1,
	MTF_NORMAL = 2,
	MTF_HARD = 4,
	MTF_AMBUSH = 8,      // Deaf monsters/do not react to sound.
	MTF_MULTI = 16,     // Multiplayer specific
	MTF_SPAWN = 32,    // Don't spawn until triggered in level
	MTF_ONTOUCH = 64,    // Trigger something when picked up
	MTF_ONDEATH = 128,   // Trigger something when killed
	MTF_SECRET = 256,    // Count as secret for intermission when picked up
	MTF_NOINFIGHTING = 512,    // Ignore other attackers
	MTF_NODEATHMATCH = 1024,   // Don't spawn in deathmatch games
	MTF_NONETGAME = 2048,  // Don't spawn in standard netgame mode
	MTF_NIGHTMARE = 4096,   // [kex] Nightmare thing
};

typedef enum {
	sk_baby,
	sk_easy,
	sk_medium,
	sk_hard,
	sk_nightmare
} skill_t;

//
// Key cards.
//
typedef enum {
	it_bluecard,
	it_yellowcard,
	it_redcard,
	it_blueskull,
	it_yellowskull,
	it_redskull,

	NUMCARDS
} card_t;

// The defined weapons,
//  including a marker indicating
//  user has not changed weapon.
typedef enum {
	wp_chainsaw,
	wp_fist,
	wp_pistol,
	wp_shotgun,
	wp_supershotgun,
	wp_chaingun,
	wp_missile,
	wp_plasma,
	wp_bfg,
	wp_laser,
	NUMWEAPONS,

	// No pending weapon change.
	wp_nochange
} weapontype_t;

// Ammunition types defined.
typedef enum {
	am_clip,    // Pistol / chaingun ammo.
	am_shell,   // Shotgun / double barreled shotgun.
	am_cell,    // Plasma rifle, BFG.
	am_misl,    // Missile launcher.
	NUMAMMO,
	am_noammo    // Unlimited for chainsaw / fist.
} ammotype_t;

// Power up artifacts.
typedef enum {
	pw_invulnerability,
	pw_strength,
	pw_invisibility,
	pw_ironfeet,
	pw_allmap,
	pw_infrared,
	NUMPOWERS
} powertype_t;

#define BONUSADD    4

//
// Power up durations,
//  how many seconds till expiration,
//
typedef enum {
	INVULNTICS = (30 * TICRATE),
	INVISTICS = (60 * TICRATE),
	INFRATICS = (120 * TICRATE),
	IRONTICS = (60 * TICRATE),
	STRTICS = (3 * TICRATE)
} powerduration_t;

// 20120209 villsa - game flags
enum {
	GF_NOMONSTERS = (1 << 0),
	GF_FASTMONSTERS = (1 << 1),
	GF_RESPAWNMONSTERS = (1 << 2),
	GF_RESPAWNPICKUPS = (1 << 3),
	GF_ALLOWJUMP = (1 << 4),
	GF_LOCKMONSTERS = (1 << 6),
	GF_ALLOWCHEATS = (1 << 7),
	GF_FRIENDLYFIRE = (1 << 8),
	GF_KEEPITEMS = (1 << 9),
};

// 20120209 villsa - compatibility flags
enum {
	COMPATF_MOBJPASS = (1 << 1)     // allow mobjs to stand on top one another
};

extern boolean windowpause;

//
// DOOM keyboard definition.
// This is the stuff configured by Setup.Exe.
// Most key data are simple ascii (uppercased).
//
enum
{
	KEY_RIGHTARROW = 0xae,
	KEY_LEFTARROW = 0xac,
	KEY_UPARROW = 0xad,
	KEY_DOWNARROW = 0xaf,
	KEY_ESCAPE = 27,
	KEY_ENTER = 13,
	KEY_TAB = 9,
	KEY_F1 = (0x80+0x3b),
	KEY_F2 = (0x80+0x3c),
	KEY_F3 =  (0x80+0x3d),
	KEY_F4 = (0x80+0x3e),
	KEY_F5 = (0x80+0x3f),
	KEY_F6 = (0x80+0x40),
	KEY_F7 = (0x80+0x41),
	KEY_F8 = (0x80+0x42),
	KEY_F9 = (0x80+0x43),
	KEY_F10 = (0x80+0x44),
	KEY_F11 = (0x80+0x57),
	KEY_F12 = (0x80+0x58),
	KEY_BACKSPACE = 127,
	KEY_PAUSE = 0xff,
	KEY_KPSTAR = 298,
	KEY_KPPLUS = 299,
	KEY_EQUALS = 0x3d,
	KEY_MINUS = 0x2d,
	KEY_SHIFT = (0x80+0x36),
	KEY_CTRL = (0x80+0x1d),
	KEY_ALT = (0x80+0x38),
	KEY_RSHIFT = (0x80+0x36),
	KEY_RCTRL = (0x80+0x1d),
	KEY_RALT = (0x80+0x38),
	KEY_INSERT = 0xd2,
	KEY_HOME = 0xc7,
	KEY_PAGEUP = 0xc9,
	KEY_PAGEDOWN = 0xd1,
	KEY_DEL = 0xc8,
	KEY_END = 0xcf,
	KEY_SCROLLLOCK = 0xc6,
	KEY_SPACEBAR = 0x20,
	KEY_KEYPADENTER = 269,
	KEY_KEYPADMULTIPLY = 298,
	KEY_KEYPADPLUS = 299,
	KEY_NUMLOCK = 300,
	KEY_KEYPADMINUS =  301,
	KEY_KEYPADPERIOD = 302,
	KEY_KEYPADDIVIDE = 303,
	KEY_KEYPAD0 = 304,
	KEY_KEYPAD1 = 305,
	KEY_KEYPAD2 = 306,
	KEY_KEYPAD3 = 307,
	KEY_KEYPAD4 = 308,
	KEY_KEYPAD5 = 309,
	KEY_KEYPAD6 = 310,
	KEY_KEYPAD7 = 311,
	KEY_KEYPAD8 = 312,
	KEY_KEYPAD9 = 313,
	KEY_MWHEELUP = (0x80 + 0x6b),
	KEY_MWHEELDOWN = (0x80 + 0x6c),
	GAMEPAD_INVALID = 0,
	GAMEPAD_A = 400, // start after KEY_
	GAMEPAD_B,
	GAMEPAD_X,
	GAMEPAD_Y,
	GAMEPAD_BACK,
	GAMEPAD_GUIDE,
	GAMEPAD_START,
	GAMEPAD_LSTICK,
	GAMEPAD_RSTICK,
	GAMEPAD_LSHOULDER,
	GAMEPAD_RSHOULDER,
	GAMEPAD_LTRIGGER,
	GAMEPAD_RTRIGGER,
	GAMEPAD_DPAD_UP,
	GAMEPAD_DPAD_DOWN,
	GAMEPAD_DPAD_LEFT,
	GAMEPAD_DPAD_RIGHT,
	GAMEPAD_LEFT_STICK,
	GAMEPAD_RIGHT_STICK
};

#include "doomtype.h"
#include "gl_utils.h"
//code assumes MOUSE_BUTTONS<10
#define MOUSE_BUTTONS        6

#endif          // __DOOMDEF__
