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
//
// DESCRIPTION:
//      Cheat sequence checking.
//
//-----------------------------------------------------------------------------

#include <stdlib.h>
#include <ctype.h>
#include "g_game.h"
#include "d_englsh.h"
#include "doomstat.h"
#include "m_cheat.h"
#include "p_inter.h"
#include "d_devstat.h"
#include "m_misc.h"
#include "am_map.h"
#include "m_password.h"
#include "st_stuff.h"
#include "deh_main.h"
#include "deh_misc.h"
#include "deh_str.h"

typedef struct {
	const int8_t* cheat;
	void (* const func)(player_t*, int8_t[4]);
	const int arg;
	uint32_t code, mask;
} cheatinfo_t;

static void M_CheatFa(player_t* player, int8_t dat[4]);
static void M_CheatBerserk(player_t* player, int8_t dat[4]);
static void M_CheatWarp(player_t* player, int8_t dat[4]);
static void M_CheatMyPos(player_t* player, int8_t dat[4]);
static void M_CheatAllMap(player_t* player, int8_t dat[4]);

cheatinfo_t cheat[] = {
	{   "iddqd",    M_CheatGod,         0   },
	{   "idfa",     M_CheatFa,          0   },
	{   "idkfa",    M_CheatKfa,         0   },
	{   "idclip",   M_CheatClip,        0   },
	{   "idclev",   M_CheatWarp,        -2  },
	{   "idpos",    M_CheatMyPos,       0   },
	{   "exm",		M_CheatAllMap,      0   },
	{   "exr",		M_CheatBerserk,     0   },
	{   "exw",		M_CheatGiveWeapon,  -1  },
	{   "exg",		M_CheatGiveKey,     -1  },
	{   "exk",		M_CheatBoyISuck,    0   },
	{   "exa",		M_CheatArtifacts,   -1  },
	{   NULL,       NULL,               0   }
};

void M_CheatGod(player_t* player, int8_t dat[4]) {
	player->cheats ^= CF_GODMODE;
	if (player->cheats & CF_GODMODE) {
		if (player->mo) {
			player->mo->health = 100;
		}

		player->health = deh_god_mode_health;
		player->message = STSTR_DQDON;
	}
	else {
		player->message = STSTR_DQDOFF;
	}
}

static void M_CheatFa(player_t* player, int8_t dat[4]) {
	int i;

	player->armorpoints = deh_idfa_armor;
	player->armortype = deh_idfa_armor_class;

	for (i = 0; i < NUMWEAPONS; i++) {
		player->weaponowned[i] = true;
	}

	for (i = 0; i < NUMAMMO; i++) {
		player->ammo[i] = player->maxammo[i];
	}

	player->message = STSTR_FAADDED;
}

void M_CheatKfa(player_t* player, int8_t dat[4]) {
	int i;

	player->armorpoints = deh_idkfa_armor;
	player->armortype = deh_idkfa_armor_class;

	for (i = 0; i < NUMWEAPONS; i++) {
		player->weaponowned[i] = true;
	}

	for (i = 0; i < NUMAMMO; i++) {
		player->ammo[i] = player->maxammo[i];
	}

	for (i = 0; i < NUMCARDS; i++) {
		player->cards[i] = true;
	}

	player->message = STSTR_KFAADDED;
}

void M_CheatClip(player_t* player, int8_t dat[4]) {
	player->cheats ^= CF_NOCLIP;

	if (player->cheats & CF_NOCLIP) {
		player->message = STSTR_NCON;
	}
	else {
		player->message = STSTR_NCOFF;
	}
}

static void M_CheatBerserk(player_t* player, int8_t dat[4]) {
	P_GivePower(player, pw_strength);
	player->message = GOTBERSERK;
}

CVAR_EXTERNAL(sv_skill);
static void M_CheatWarp(player_t* player, int8_t dat[4]) {
	char	lumpname[9];
	int		lumpnum;
	int map;
	map = datoi(dat);
	gameskill = (int)sv_skill.value;
	gamemap = nextmap = map;

	if (map < 1)
	{
		return;
	}
	if (map < 10)
	{
		DEH_snprintf(lumpname, 9, "MAP0%i", map);
	}
	else {
		DEH_snprintf(lumpname, 9, "MAP%i", map);
	}
	lumpnum = map ? W_GetNumForName(lumpname) : W_CheckNumForName(lumpname);

	if (lumpnum)
	{
		// So be it.
		G_DeferedInitNew(gameskill, map);
		dmemset(passwordData, 0xff, 16);
	}
}

static void M_CheatMyPos(player_t* player, int8_t dat[4]) {
	_dprintf("ang = %d; x,y = (%d, %d)",
		(int)(players[consoleplayer].mo->angle * (float)180 / ANG180),
		F2INT(players[consoleplayer].mo->x),
		F2INT(players[consoleplayer].mo->y));
}

static void M_CheatAllMap(player_t* player, int8_t dat[4]) {
	amCheating = (amCheating + 1) % 3;
}

void M_CheatGiveWeapon(player_t* player, int8_t dat[4]) {
	int8_t c = dat[0];
	int w = datoi(&c);

	static int8_t* WeapGotNames[8] = {
		GOTCHAINSAW,
		GOTSHOTGUN,
		GOTSHOTGUN2,
		GOTCHAINGUN,
		GOTLAUNCHER,
		GOTPLASMA,
		GOTBFG9000,
		GOTLASER
	};

	static weapontype_t WeapTypes[8] = {
		wp_chainsaw,
		wp_shotgun,
		wp_supershotgun,
		wp_chaingun,
		wp_missile,
		wp_plasma,
		wp_bfg,
		wp_laser
	};

	if (!w || w >= 9) {
		return;
	}

	if (!P_GiveWeapon(player, NULL, WeapTypes[w - 1], false)) {
		return;
	}

	player->message = WeapGotNames[w - 1];
}

void M_CheatGiveKey(player_t* player, int8_t dat[4]) {
	int8_t c = dat[0];
	int k = datoi(&c);

	if (!k || k > NUMCARDS) {
		return;
	}

	player->cards[k - 1] ^= 1;
	player->message = player->cards[k - 1] ? "Key Added" : "Key Removed";
}

void M_CheatBoyISuck(player_t* player, int8_t dat[4]) {
	D_BoyISuck();
	player->message = DEVKILLALL;
}

void M_CheatArtifacts(player_t* player, int8_t dat[4]) {
	int8_t c = dat[0];
	int a = datoi(&c);

	static int8_t* ArtiGotNames[3] = {
		GOTARTIFACT1,
		GOTARTIFACT2,
		GOTARTIFACT3
	};

	if (!a || a > 3) {
		return;
	}

	a = a - 1;

	player->artifacts |= (1 << a);
	player->message = ArtiGotNames[a];
}

#define CHEAT_ARGS_MAX 8  /* Maximum number of args at end of cheats */

static dboolean M_FindCheats(player_t* plyr, int key) {
	static unsigned long sr;
	static int8_t argbuf[CHEAT_ARGS_MAX + 1], * arg;
	static int init, argsleft, cht;
	int i, ret, matchedbefore;
	int8_t buff[4];

	if (argsleft) {
		// store key in arg buffer
		*arg++ = tolower(key);

		// if last key in arg list,
		if (!--argsleft) {
			cheat[cht].func(plyr, argbuf);    // process the arg buffer
		}

		// affirmative response
		return 1;
	}

	key = tolower(key) - 'a';

	// ignore most non-alpha cheat letters
	if (key < 0 || key >= 32) {
		// clear shift register
		sr = 0;
		return 0;
	}

	// initialize aux entries of table
	if (!init) {
		init = 1;
		for (i = 0; cheat[i].cheat; i++) {
			intptr_t c = 0, m = 0;
			const int8_t* p;

			for (p = cheat[i].cheat; *p; p++) {
				unsigned key = tolower(*p) - 'a'; // convert to 0-31

				// ignore most non-alpha cheat letters
				if (key >= 32) {
					continue;
				}

				// shift key into code
				c = (c << 5) + key;

				// shift 1's into mask
				m = (m << 5) + 31;
			}

			// code for this cheat key
			cheat[i].code = c;

			// mask for this cheat key
			cheat[i].mask = m;
		}
	}

	// shift this key into shift register
	sr = (sr << 5) + key;

	for (matchedbefore = ret = i = 0; cheat[i].cheat; i++) {
		if ((sr & cheat[i].mask) == cheat[i].code) {
			// if additional args are required
			if (cheat[i].arg < 0) {
				// remember this cheat code
				cht = i;

				// point to start of arg buffer
				arg = argbuf;

				// number of args expected
				argsleft = -cheat[i].arg;

				// responder has eaten key
				ret = 1;
			}
			else {
				// allow only one cheat at a time
				if (!matchedbefore) {
					// responder has eaten key
					matchedbefore = ret = 1;
					sprintf(buff, "%i", cheat[i].arg);

					// call cheat handler
					if (netgame) {
						NET_CL_SendCheat(consoleplayer, i, buff);
					}
					else {
						cheat[i].func(plyr, buff);
					}
				}
			}
		}
	}

	return ret;
}

void M_ParseNetCheat(int player, int type, int8_t* buff) {
	player_t* p = &players[player];

	cheat[type].func(p, buff);

	if (p->message && player != consoleplayer) {
		ST_Notification(p->message);
	}
}

void M_CheatProcess(player_t* plyr, event_t* ev) {
	if (netgame && !(gameflags & GF_ALLOWCHEATS)) {
		return;
	}

	// if a user keypress...
	if (ev->type == ev_keydown) {
		M_FindCheats(plyr, ev->data1);
	}
}
