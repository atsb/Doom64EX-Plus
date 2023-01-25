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
//    Switches, buttons. Two-state animation. Exits.
//
//-----------------------------------------------------------------------------

#include "i_system.h"
#include "doomdef.h"
#include "p_local.h"
#include "w_wad.h"
#include "g_game.h"
#include "s_sound.h"
#include "sounds.h"
#include "doomstat.h"
#include "gl_texture.h"
#include "r_local.h"
#include "z_zone.h"

button_t buttonlist[MAXBUTTONS];

//
// P_StartButton
// Start a button counting down till it turns off.
//

void P_StartButton(line_t* line, bwhere_e w, int texture, int time)
{
	int    i;

	for (i = 0; i < MAXBUTTONS; i++)
	{
		if (!buttonlist[i].btimer)
		{
			buttonlist[i].side = &sides[line->sidenum[0]];
			buttonlist[i].where = w;
			buttonlist[i].btexture = texture;
			buttonlist[i].btimer = time;
			buttonlist[i].soundorg = (mobj_t*)&line->frontsector->soundorg;
			return;
		}
	}
}

//
// P_ChangeSwitchTexture
// Function that changes wall texture.
// Tell it if switch is ok to use again (1=yes, it's a button).
//

void P_ChangeSwitchTexture(line_t* line, int useAgain) {
	int sound;
	int swx;

	if (SPECIALMASK(line->special) == 52 || SPECIALMASK(line->special) == 124) {
		sound = sfx_switch2;
	}
	else {
		sound = sfx_switch1;
	}

	if (SWITCHMASK(line->flags) == ML_SWITCHX04) {

		S_StartSound(buttonlist->soundorg, sound);

		swx = sides[line->sidenum[0]].bottomtexture;
		sides[line->sidenum[0]].bottomtexture = ((swx - swx_start) ^ 1) + swx_start;

		if (useAgain) {
			P_StartButton(line, bottom, swx, BUTTONTIME);
		}

		return;
	}
	else if (SWITCHMASK(line->flags) == ML_SWITCHX02) {

		S_StartSound(buttonlist->soundorg, sound);

		swx = sides[line->sidenum[0]].toptexture;
		sides[line->sidenum[0]].toptexture = ((swx - swx_start) ^ 1) + swx_start;

		if (useAgain) {
			P_StartButton(line, top, swx, BUTTONTIME);
		}

		return;
	}
	else if (SWITCHMASK(line->flags) == (ML_SWITCHX02 | ML_SWITCHX04))
	{
		S_StartSound(buttonlist->soundorg, sound);

		swx = sides[line->sidenum[0]].midtexture;
		sides[line->sidenum[0]].midtexture = ((swx - swx_start) ^ 1) + swx_start;

		if (useAgain) {
			P_StartButton(line, middle, swx, BUTTONTIME);
		}

		return;
	}
}
