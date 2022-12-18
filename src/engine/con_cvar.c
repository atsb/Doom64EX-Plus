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
// DESCRIPTION: Console cvar functionality (from Quake)
//
//-----------------------------------------------------------------------------

#include "doomstat.h"
#include "i_video.h"
#include "i_sdlinput.h"
#include "con_console.h"
#include "z_zone.h"
#include "i_system.h"
#include "con_cvar.h"
#include "am_map.h"
#include "s_sound.h"
#include "r_main.h"
#include "m_menu.h"
#include "p_setup.h"
#include "st_stuff.h"
#include "g_game.h"
#include "g_actions.h"

cvar_t* cvarcap;

//
// CMD_ListCvars
//

static CMD(ListCvars) {
	cvar_t* var;

	CON_Printf(GREEN, "Available cvars:\n");

	for (var = cvarcap; var; var = var->next) {
		CON_Printf(AQUA, "%s\n", var->name);
	}
}

//
// CON_CvarGet
//

cvar_t* CON_CvarGet(int8_t* name) {
	cvar_t* var;

	for (var = cvarcap; var; var = var->next) {
		if (!dstrcmp(name, var->name)) {
			return var;
		}
	}

	return NULL;
}

//
// CON_CvarValue
//

float CON_CvarValue(int8_t* name) {
	cvar_t* var;

	var = CON_CvarGet(name);
	if (!var) {
		return 0;
	}

	return datof(var->string);
}

//
// CON_CvarString
//

int8_t* CON_CvarString(int8_t* name) {
	cvar_t* var;

	var = CON_CvarGet(name);
	if (!var) {
		return "";
	}

	return var->string;
}

//
// CON_CvarSet
//

void CON_CvarSet(int8_t* var_name, int8_t* value) {
	cvar_t* var;
	dboolean changed;

	var = CON_CvarGet(var_name);
	if (!var) {
		// there is an error in C code if this happens
		CON_Printf(WHITE, "CON_CvarSet: variable %s not found\n", var_name);
		return;
	}

	changed = dstrcmp(var->string, value);

	Z_Free(var->string);    // free the old value string

	var->string = Z_Malloc(dstrlen(value) + 1, PU_STATIC, 0);
	dstrcpy(var->string, value);
	var->value = datof(var->string);

	if (var->callback) {
		var->callback(var);
	}
}

//
// CON_CvarSetValue
//

void CON_CvarSetValue(int8_t* var_name, float value) {
	int8_t val[32];

	sprintf(val, "%f", value);
	CON_CvarSet(var_name, val);
}

//
// CON_CvarRegister
//

void CON_CvarRegister(cvar_t* variable) {
	int8_t* oldstr;

	// first check to see if it has allready been defined
	if (CON_CvarGet(variable->name)) {
		CON_Printf(WHITE, "CON_CvarRegister: Can't register variable %s, already defined\n", variable->name);
		return;
	}

	// copy the value off, because future sets will Z_Free it
	oldstr = variable->string;
	variable->string = Z_Malloc(dstrlen(variable->string) + 1, PU_STATIC, 0);
	dstrcpy(variable->string, oldstr);
	variable->value = datof(variable->string);
	variable->defvalue = Z_Malloc(dstrlen(variable->string) + 1, PU_STATIC, 0);
	dstrcpy(variable->defvalue, variable->string);

	// link the variable in
	variable->next = cvarcap;
	cvarcap = variable;
}

//
// CON_CvarInit
//

void CON_CvarInit(void) {
	AM_RegisterCvars();
	R_RegisterCvars();
	V_RegisterCvars();
	ST_RegisterCvars();
	S_RegisterCvars();
	I_RegisterCvars();
	ISDL_RegisterKeyCvars();
	M_RegisterCvars();
	P_RegisterCvars();
	G_RegisterCvars();

	G_AddCommand("listcvars", CMD_ListCvars, 0);
}
