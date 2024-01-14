// Emacs style mode select   -*- C -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 1993-1997 Id Software, Inc.
// Copyright(C) 2007-2013 Samuel Villarreal
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
// DESCRIPTION: Demo System
//
//-----------------------------------------------------------------------------

#include "doomdef.h"
#include "doomstat.h"
#include "z_zone.h"
#include "p_tick.h"
#include "g_local.h"
#include "g_demo.h"
#include "m_misc.h"
#include "m_random.h"
#include "con_console.h"

#ifdef _WIN32
#include "i_opndir.h"
#endif

#if !defined _WIN32
#include <unistd.h>
#endif

void        G_DoLoadLevel(void);
boolean    G_CheckDemoStatus(void);
void        G_ReadDemoTiccmd(ticcmd_t* cmd);
void        G_WriteDemoTiccmd(ticcmd_t* cmd);

FILE* demofp;
byte* demo_p;
char            demoname[256];
boolean        demorecording = false;
boolean        demoplayback = false;
boolean        netdemo = false;
byte* demobuffer;
byte* demoend;
boolean        singledemo = false;    // quit after playing a demo from cmdline
boolean        endDemo;
boolean        iwadDemo = false;

extern int      starttime;

//
// DEMO RECORDING
//

//
// G_ReadDemoTiccmd
//

void G_ReadDemoTiccmd(ticcmd_t* cmd) {
	unsigned int lowbyte;

	if (*demo_p == DEMOMARKER) {
		// end of demo data stream
		G_CheckDemoStatus();
		return;
	}

	cmd->forwardmove = ((char)*demo_p++);
	cmd->sidemove = ((char)*demo_p++);
	lowbyte = (unsigned char)(*demo_p++);
	cmd->angleturn = (((int)(*demo_p++)) << 8) + lowbyte;
	lowbyte = (unsigned char)(*demo_p++);
	cmd->pitch = (((int)(*demo_p++)) << 8) + lowbyte;
	cmd->buttons = (unsigned char)*demo_p++;
	cmd->buttons2 = (unsigned char)*demo_p++;
}

//
// G_WriteDemoTiccmd
//

void G_WriteDemoTiccmd(ticcmd_t* cmd) {
	char buf[8];
	short angleturn;
	short pitch;
	char* p = buf;

	angleturn = cmd->angleturn;
	pitch = cmd->pitch;

	*p++ = cmd->forwardmove;
	*p++ = cmd->sidemove;
	*p++ = angleturn & 0xff;
	*p++ = (angleturn >> 8) & 0xff;
	*p++ = pitch & 0xff;
	*p++ = (pitch >> 8) & 0xff;
	*p++ = cmd->buttons;
	*p++ = cmd->buttons2;

	if (fwrite(buf, p - buf, 1, demofp) != 1) {
		I_Error("G_WriteDemoTiccmd: error writing demo");
	}

	// alias demo_p to it so we can read it back
	demo_p = (byte*)buf;
	G_ReadDemoTiccmd(cmd);    // make SURE it is exactly the same
}

//
// G_RecordDemo
//

void G_RecordDemo(const char* name) {
	byte* demostart, * dm_p;
	int i;

	demofp = NULL;
	endDemo = false;

	dstrcpy(demoname, name);
	dstrcat(demoname, ".lmp");
#ifdef _WIN32
	if (_access(demoname, F_OK))
#else
	if (access(demoname, F_OK))
#endif
	{
		demofp = fopen(demoname, "wb");
	}
	else {
		int demonum = 0;

		while (demonum < 10000) {
			sprintf(demoname, "%s%i.lmp", name, demonum);
#ifdef _WIN32
			if (_access(demoname, F_OK))
#else
			if (access(demoname, F_OK))
#endif
			{
				demofp = fopen(demoname, "wb");
				break;
			}
			demonum++;
		}

		// so yeah... dunno how to properly handle this...
		if (demonum >= 10000) {
			I_Error("G_RecordDemo: file %s already exists", demoname);
			return;
		}
	}

	CON_DPrintf("--------Recording %s--------\n", demoname);

	demostart = dm_p = (byte*)malloc(1000);

	G_InitNew(startskill, startmap);

	*dm_p++ = gameskill;
	*dm_p++ = gamemap;
	*dm_p++ = deathmatch;
	*dm_p++ = respawnparm;
	*dm_p++ = respawnitem;
	*dm_p++ = fastparm;
	*dm_p++ = nomonsters;
	*dm_p++ = consoleplayer;

	*dm_p++ = (byte)((gameflags >> 24) & 0xff);
	*dm_p++ = (byte)((gameflags >> 16) & 0xff);
	*dm_p++ = (byte)((gameflags >> 8) & 0xff);
	*dm_p++ = (byte)(gameflags & 0xff);

	*dm_p++ = (byte)((compatflags >> 24) & 0xff);
	*dm_p++ = (byte)((compatflags >> 16) & 0xff);
	*dm_p++ = (byte)((compatflags >> 8) & 0xff);
	*dm_p++ = (byte)(compatflags & 0xff);

	for (i = 0; i < MAXPLAYERS; i++) {
		*dm_p++ = playeringame[i];
	}

	if (fwrite(demostart, 1, dm_p - demostart, demofp) != (unsigned int)(dm_p - demostart)) {
		I_Error("G_RecordDemo: Error writing demo header");
	}

	free(demostart);

	demorecording = true;
	usergame = false;

	G_RunGame();
	G_CheckDemoStatus();
}

//
// G_PlayDemo
//

void G_PlayDemo(const char* name) {
	int i;
	int p;
	char filename[256];

	gameaction = ga_nothing;
	endDemo = false;

	p = M_CheckParm("-playdemo");
	if (p && p < myargc - 1) {
		// 20120107 bkw: add .lmp extension if missing.
		if (dstrrchr(myargv[p + 1], '.')) {
			dstrcpy(filename, myargv[p + 1]);
		}
		else {
			dsprintf(filename, "%s.lmp", myargv[p + 1]);
		}

		CON_DPrintf("--------Reading demo %s--------\n", filename);
		if (M_ReadFile(filename, &demobuffer) == -1) {
			gameaction = ga_exitdemo;
			return;
		}

		demo_p = demobuffer;
	}
	else {
		if (W_CheckNumForName(name) == -1) {
			gameaction = ga_exitdemo;
			return;
		}

		CON_DPrintf("--------Playing demo %s--------\n", name);
		demobuffer = demo_p = (byte*)W_CacheLumpName(name, PU_STATIC);
	}

	G_SaveDefaults();

	startskill = *demo_p++;
	startmap = *demo_p++;
	deathmatch = *demo_p++;
	respawnparm = *demo_p++;
	respawnitem = *demo_p++;
	fastparm = *demo_p++;
	nomonsters = *demo_p++;
	consoleplayer = *demo_p++;

	gameflags = *demo_p++ & 0xff;
	gameflags <<= 8;
	gameflags += *demo_p++ & 0xff;
	gameflags <<= 8;
	gameflags += *demo_p++ & 0xff;
	gameflags <<= 8;
	gameflags += *demo_p++ & 0xff;

	compatflags = *demo_p++ & 0xff;
	compatflags <<= 8;
	compatflags += *demo_p++ & 0xff;
	compatflags <<= 8;
	compatflags += *demo_p++ & 0xff;
	compatflags <<= 8;
	compatflags += *demo_p++ & 0xff;

	for (i = 0; i < MAXPLAYERS; i++) {
		playeringame[i] = *demo_p++;
	}

	G_InitNew(startskill, startmap);

	if (playeringame[1]) {
		netgame = true;
		netdemo = true;
	}

	precache = true;
	usergame = false;
	demoplayback = true;

	G_RunGame();
	iwadDemo = false;
}

//
// G_CheckDemoStatus
// Called after a death or level completion to allow demos to be cleaned up
// Returns true if a new demo loop action will take place
//

boolean G_CheckDemoStatus(void) {
	if (endDemo) {
		demorecording = false;
		fputc(DEMOMARKER, demofp);
		CON_Printf(WHITE, "G_CheckDemoStatus: Demo recorded\n");
		fclose(demofp);
		endDemo = false;
		return false;
	}

	if (demoplayback) {
		if (singledemo) {
			I_Quit();
		}

		netdemo = false;
		netgame = false;
		deathmatch = false;
		playeringame[1] = playeringame[2] = playeringame[3] = 0;
		respawnparm = false;
		respawnitem = false;
		fastparm = false;
		nomonsters = false;
		consoleplayer = 0;
		gameaction = ga_exitdemo;
		endDemo = false;

		G_ReloadDefaults();
		return true;
	}

	return false;
}

/* VANILLA */

int G_PlayDemoPtr(int skill, int map) // 800049D0
{
	int		exit;

	demobuffer = demo_p;

	/* play demo game */
	G_InitNew(skill, map);
	G_DoLoadLevel();
	demoplayback = true;
	exit = D_MiniLoop(P_Start, P_Stop, P_Drawer, P_Ticker);
	demoplayback = false;

	return exit;
}

int D_RunDemo(char* name, skill_t skill, int map) // 8002B2D0
{
	int lump;
	int exit;

	demo_p = Z_Malloc(16000, PU_STATIC, NULL);

	lump = W_GetNumForName(name);
	W_ReadLump(lump, demo_p);
	exit = G_PlayDemoPtr(skill, map);

	return exit;
}
