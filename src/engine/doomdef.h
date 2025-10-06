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

#ifndef __DOOMDEF__
#define __DOOMDEF__

#include "doomtype.h"
#include "tables.h"

// build version
extern const char version_date[];

void*		dmemcpy(void* s1, const void* s2, unsigned int n);
void*		dmemset(void* s, int c, unsigned int n);
char*		dstrcpy(char* dest, const char* src);
void        dstrncpy(char* dest, const char* src, int maxcount);
int         dstrcmp(const char* s1, const char* s2);
boolean     dstreq(const char* s1, const char* s2);
int         dstrncmp(const char* s1, const char* s2, int len);
int         dstricmp(const char* s1, const char* s2);
int         dstrnicmp(const char* s1, const char* s2, int len);
void        dstrupr(char* s);
void        dstrlwr(char* s);
int         dstrlen(const char* string);
char*		dstrrchr(char* s, char c);
void        dstrcat(char* dest, const char* src);
char*		dstrstr(char* s1, char* s2);
boolean		dstrisempty(char* s);
int         datoi(const char* str);
float       datof(char* str);
int         dhtoi(char* str);
boolean		dfcmp(float f1, float f2);

extern int D_abs(int v);
extern float D_fabs(float x);

#define dcos(angle) finecosine[(angle) >> ANGLETOFINESHIFT]
#define dsin(angle) finesine[(angle) >> ANGLETOFINESHIFT]

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
// cast first argument a to rcolor because: left shift of 255 by 24 places cannot be represented in type 'int'
#ifdef SYS_BIG_ENDIAN
#define D_RGBA(r,g,b,a) ((rcolor)((((rcolor)((r)&0xff))<<24)|(((g)&0xff)<<16)|(((b)&0xff)<<8)|((a)&0xff)))
#else
#define D_RGBA(r,g,b,a) ((rcolor)((((rcolor)((a)&0xff))<<24)|(((b)&0xff)<<16)|(((g)&0xff)<<8)|((r)&0xff)))
#endif

// basic color definitions
#define WHITE       D_RGBA(255, 255, 255, 255)
#define RED         D_RGBA(255, 0, 0, 255)
#define GREEN       D_RGBA(0, 255, 0, 255)
#define BLUE        D_RGBA(0, 0, 255, 255)
#define YELLOW      D_RGBA(255, 255, 0, 255)
#define AQUA        D_RGBA(0, 255, 255, 255)
#define REDALPHA(x)     D_RGBA(255, 0, 0, (x))
#define WHITEALPHA(x)   D_RGBA(255, 255, 255, (x))

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
#define MTF_DROPOFF         16384  // This allows jumps from high places.

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
	wp_nochange,

	// Key bindings only.
	wp_onlyshotgun,
	wp_onlyfist,
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

extern boolean windowpause;

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

#define KEY_CONSOLE				'`'

//code assumes MOUSE_BUTTONS<10
#define MOUSE_BUTTONS        9

// all the required data files from the Remaster
#define IWAD_FILENAME			"DOOM64.WAD"
#define DLS_FILENAME			"DOOMSND.DLS"
#define KPF_FILENAME			"Doom64.kpf"

#endif          // __DOOMDEF__
