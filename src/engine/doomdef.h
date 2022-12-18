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
#include <windows.h>
#include <wtypes.h>//only for GUID type

#endif // _WIN32

#include <stdio.h>

#include "doomtype.h"
#include "d_keywds.h"
#include "tables.h"

// build version
extern const int8_t version_date[];

void        _dprintf(const int8_t* s, ...);
void* dmemcpy(void* s1, const void* s2, size_t n);
void* dmemset(void* s, dword c, size_t n);
int8_t* dstrcpy(int8_t* dest, const int8_t* src);
void        dstrncpy(int8_t* dest, const int8_t* src, int maxcount);
int         dstrcmp(const int8_t* s1, const int8_t* s2);
int         dstrncmp(const int8_t* s1, const int8_t* s2, int len);
int         dstricmp(const int8_t* s1, const int8_t* s2);
int         dstrnicmp(const int8_t* s1, const int8_t* s2, int len);
void        dstrupr(int8_t* s);
void        dstrlwr(int8_t* s);
int         dstrlen(const int8_t* string);
int8_t* dstrrchr(int8_t* s, int8_t c);
void        dstrcat(int8_t* dest, const int8_t* src);
int8_t* dstrstr(int8_t* s1, int8_t* s2);
int         datoi(const int8_t* str);
float       datof(int8_t* str);
int         dhtoi(int8_t* str);
dboolean    dfcmp(float f1, float f2);
int         dsprintf(int8_t* buf, const int8_t* format, ...);
int         dsnprintf(int8_t* src, size_t n, const int8_t* str, ...);

extern int D_abs(int x);
extern float D_fabs(float x);

#define dcos(angle) finecosine[(angle) >> ANGLETOFINESHIFT]
#define dsin(angle) finesine[(angle) >> ANGLETOFINESHIFT]

#include "dgl.h"

// #define macros to provide functions missing in Windows.
// Outside Windows, we use strings.h for str[n]casecmp.

#ifdef _WIN32

#define snprintf _snprintf
#define vsnprintf _vsnprintf

#else

#include <strings.h>

#endif

//
// The packed attribute forces structures to be packed into the minimum
// space necessary.  If this is not done, the compiler may align structure
// fields differently to optimise memory access, inflating the overall
// structure size.  It is important to use the packed attribute on certain
// structures where alignment is important, particularly data read/written
// to disk.
//

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
#define MTF_EASY            1
#define MTF_NORMAL          2
#define MTF_HARD            4
#define MTF_AMBUSH          8      // Deaf monsters/do not react to sound.
#define MTF_MULTI           16     // Multiplayer specific
#define MTF_SPAWN           32     // Don't spawn until triggered in level
#define MTF_ONTOUCH         64     // Trigger something when picked up
#define MTF_ONDEATH         128    // Trigger something when killed
#define MTF_SECRET          256    // Count as secret for intermission when picked up
#define MTF_NOINFIGHTING    512    // Ignore other attackers
#define MTF_NODEATHMATCH    1024   // Don't spawn in deathmatch games
#define MTF_NONETGAME       2048   // Don't spawn in standard netgame mode
#define MTF_NIGHTMARE       4096   // [kex] Nightmare thing

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
	GF_ALLOWAUTOAIM = (1 << 5),
	GF_LOCKMONSTERS = (1 << 6),
	GF_ALLOWCHEATS = (1 << 7),
	GF_FRIENDLYFIRE = (1 << 8),
	GF_KEEPITEMS = (1 << 9),
};

// 20120209 villsa - compatibility flags
enum {
	COMPATF_MOBJPASS = (1 << 1)     // allow mobjs to stand on top one another
};

extern dboolean windowpause;

//
// DOOM keyboard definition.
// This is the stuff configured by Setup.Exe.
// Most key data are simple ascii (uppercased).
//
#define KEY_RIGHTARROW          0xae
#define KEY_LEFTARROW           0xac
#define KEY_UPARROW             0xad
#define KEY_DOWNARROW           0xaf
#define KEY_ESCAPE              27
#define KEY_ENTER               13
#define KEY_TAB                 9
#define KEY_F1                  (0x80+0x3b)
#define KEY_F2                  (0x80+0x3c)
#define KEY_F3                  (0x80+0x3d)
#define KEY_F4                  (0x80+0x3e)
#define KEY_F5                  (0x80+0x3f)
#define KEY_F6                  (0x80+0x40)
#define KEY_F7                  (0x80+0x41)
#define KEY_F8                  (0x80+0x42)
#define KEY_F9                  (0x80+0x43)
#define KEY_F10                 (0x80+0x44)
#define KEY_F11                 (0x80+0x57)
#define KEY_F12                 (0x80+0x58)

#define KEY_BACKSPACE           127
#define KEY_PAUSE               0xff

#define KEY_KPSTAR              298
#define KEY_KPPLUS              299

#define KEY_EQUALS              0x3d
#define KEY_MINUS               0x2d

#define KEY_SHIFT               (0x80+0x36)
#define KEY_CTRL                (0x80+0x1d)
#define KEY_ALT                 (0x80+0x38)

#define KEY_RSHIFT              (0x80+0x36)
#define KEY_RCTRL               (0x80+0x1d)
#define KEY_RALT                (0x80+0x38)

#define KEY_INSERT              0xd2
#define KEY_HOME                0xc7
#define KEY_PAGEUP              0xc9
#define KEY_PAGEDOWN            0xd1
#define KEY_DEL                 0xc8
#define KEY_END                 0xcf
#define KEY_SCROLLLOCK          0xc6
#define KEY_SPACEBAR            0x20

#define KEY_KEYPADENTER         269
#define KEY_KEYPADMULTIPLY      298
#define KEY_KEYPADPLUS          299
#define KEY_NUMLOCK             300
#define KEY_KEYPADMINUS         301
#define KEY_KEYPADPERIOD        302
#define KEY_KEYPADDIVIDE        303
#define KEY_KEYPAD0             304
#define KEY_KEYPAD1             305
#define KEY_KEYPAD2             306
#define KEY_KEYPAD3             307
#define KEY_KEYPAD4             308
#define KEY_KEYPAD5             309
#define KEY_KEYPAD6             310
#define KEY_KEYPAD7             311
#define KEY_KEYPAD8             312
#define KEY_KEYPAD9             313

#define KEY_MWHEELUP            (0x80 + 0x6b)
#define KEY_MWHEELDOWN          (0x80 + 0x6c)

// DOOM basic types (dboolean),
//  and max/min values.
#include "doomtype.h"

// Header, generated by sound utility.
// The utility was written by Dave Taylor.
//#include "sounds.h"

//code assumes MOUSE_BUTTONS<10
#define MOUSE_BUTTONS        6

#endif          // __DOOMDEF__
