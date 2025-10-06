// Emacs style mode select   -*- C -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 1993-1997 Id Software, Inc.
// Copyright(C) 2005 Simon Howard
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
//    DOOM Network game communication and protocol,
//    all OS independend parts.
//
//-----------------------------------------------------------------------------

#include <limits.h>

#include "d_net.h"
#include "m_menu.h"
#include "i_system.h"
#include "doomdef.h"
#include "doomstat.h"
#include "tables.h"
#include "m_misc.h"
#include "i_sdlinput.h"
#include "md5.h"
#include "net_client.h"
#include "net_server.h"
#include "net_loop.h"
#include "net_query.h"
#include "net_io.h"
#include "r_main.h"


#define FEATURE_MULTIPLAYER 1

boolean    ShowGun = true;
boolean    drone = false;
boolean    net_cl_new_sync = true;    // Use new client syncronisation code
fixed_t     offsetms;

//
// NETWORKING
//
// gametic is the tic about to (or currently being) run
// maketic is the tick that hasn't had control made for it yet
// nettics[] has the maketics for all players
//
// a gametic cannot be run until nettics[] > gametic for all players

ticcmd_t    netcmds[MAXPLAYERS][BACKUPTICS];
int         nettics[MAXNETNODES];
boolean    nodeingame[MAXNETNODES];        // set false as nodes leave game
boolean    remoteresend[MAXNETNODES];      // set when local needs tics
int         resendto[MAXNETNODES];          // set when remote needs tics
int         resendcount[MAXNETNODES];

int         nodeforplayer[MAXPLAYERS];

int         maketic = 0;
int         lastnettic = 0;
int         skiptics = 0;
int         ticdup = 0;
int         maxsend = 0;    // BACKUPTICS/(2*ticdup)-1
int         extratics;

void D_ProcessEvents(void);
void G_BuildTiccmd(ticcmd_t* cmd);

boolean renderinframe = false;

//
// GetAdjustedTime
// 30 fps clock adjusted by offsetms milliseconds
//

static int GetAdjustedTime(void) {
	int time_ms;

	time_ms = I_GetTimeMS();

	if (net_cl_new_sync) {
		// Use the adjustments from net_client.c only if we are
		// using the new sync mode.

		time_ms += (offsetms / FRACUNIT);
	}

	return (time_ms * TICRATE) / 1000;
}

//
// NetUpdate
// Builds ticcmds for console player,
// sends out a packet
//
int gametime = 0;

void NetUpdate(void) {
	int nowtime;
	int newtics;
	int i;
	int gameticdiv;

	if (renderinframe) {
		return;
	}

#ifdef FEATURE_MULTIPLAYER

	// Run network subsystems

	NET_CL_Run();
	NET_SV_Run();

#endif

	// check time
	nowtime = GetAdjustedTime() / ticdup;
	newtics = nowtime - gametime;
	gametime = nowtime;

	if (skiptics <= newtics) {
		newtics -= skiptics;
		skiptics = 0;
	}
	else {
		skiptics -= newtics;
		newtics = 0;
	}

	// build new ticcmds for console player(s)
	gameticdiv = gametic / ticdup;
	for (i = 0; i < newtics; i++) {
		ticcmd_t cmd;

		I_StartTic();
		D_ProcessEvents();
		//if (maketic - gameticdiv >= BACKUPTICS/2-1)
		//    break;          // can't hold any more

		M_Ticker();

		if (drone) {  // In drone mode, do not generate any ticcmds.
			continue;
		}

		if (net_cl_new_sync) {
			// If playing single player, do not allow tics to buffer
			// up very far

			if ((!netgame || demoplayback) && maketic - gameticdiv > 2) {
				break;
			}

			// Never go more than ~200ms ahead
			if (maketic - gameticdiv > 8) {
				break;
			}
		}
		else {
			if (maketic - gameticdiv >= 5) {
				break;
			}
		}

		G_BuildTiccmd(&cmd);

#ifdef FEATURE_MULTIPLAYER

		if (netgame && !demoplayback) {
			NET_CL_SendTiccmd(&cmd, maketic);
		}

#endif
		netcmds[consoleplayer][maketic % BACKUPTICS] = cmd;

		++maketic;
		nettics[consoleplayer] = maketic;
	}
}

//
// PrintMD5Digest
//

static boolean had_warning = false;
static void PrintMD5Digest(char* s, byte* digest) {
	unsigned int i;

	I_Printf("%s: ", s);

	for (i = 0; i < sizeof(md5_digest_t); ++i) {
		I_Printf("%02x", digest[i]);
	}

	I_Printf("\n");
}

//
// CheckMD5Sums
//

static void CheckMD5Sums(void) {
	boolean correct_wad;

	if (!net_client_received_wait_data || had_warning) {
		return;
	}

	correct_wad = memcmp(net_local_wad_md5sum,
		net_server_wad_md5sum, sizeof(md5_digest_t)) == 0;

	if (correct_wad) {
		return;
	}
	else {
		I_Printf("Warning: WAD MD5 does not match server:\n");
		PrintMD5Digest("Local", net_local_wad_md5sum);
		PrintMD5Digest("Server", net_server_wad_md5sum);
		I_Printf("If you continue, this may cause your game to desync\n");
	}

	had_warning = true;
}

//
// D_NetWait
//

static void D_NetWait(void) {
	SDL_Event Event;
	unsigned int id = 0;

	if (M_CheckParm("-server") > 0) {
		I_Printf("D_NetWait: Waiting for players..\n\nWhen ready press any key to begin game..\n\n");
	}

	I_Printf("---------------------------------------------\n\n");

	while (net_waiting_for_start) {
		CheckMD5Sums();

		if (id != net_clients_in_game) {
			I_Printf("%s - %s\n", net_player_names[net_clients_in_game - 1],
				net_player_addresses[net_clients_in_game - 1]);
			id = net_clients_in_game;
		}

		NET_CL_Run();
		NET_SV_Run();

		if (!net_client_connected) {
			I_Error("D_NetWait: Disconnected from server");
		}

		I_Sleep(100);

		while (SDL_PollEvent(&Event))
			if (Event.type == SDL_EVENT_KEY_DOWN) {
				NET_CL_StartGame();
			}
	}
}

//
// D_CheckNetGame
// Works out player numbers among the net participants
//

void D_CheckNetGame(void) {
	int    i;
	// default values for single player

	consoleplayer = 0;
	netgame = false;
	ticdup = 1;
	extratics = 1;
	offsetms = 0;

	for (i = 0; i < MAXPLAYERS; i++) {
		playeringame[i] = false;
		nettics[i] = 0;
	}

	playeringame[0] = true;

#ifdef FEATURE_MULTIPLAYER
	{
		net_addr_t* addr = NULL;

		//!
		// @category net
		//
		// Start a multiplayer server, listening for connections.
		//

		if (M_CheckParm("-server") > 0) {
			NET_SV_Init();
			NET_SV_AddModule(&net_loop_server_module);

			net_loop_client_module.InitClient();
			addr = net_loop_client_module.ResolveAddress(NULL);
		}
		else {
			//!
			// @category net
			//
			// Automatically search the local LAN for a multiplayer
			// server and join it.
			//

			i = M_CheckParm("-autojoin");

			if (i > 0) {
				addr = NET_FindLANServer();

				if (addr == NULL) {
					I_Error("No server found on local LAN");
				}
			}

			//!
			// @arg <address>
			// @category net
			//
			// Connect to a multiplayer server running on the given
			// address.
			//

			i = M_CheckParm("-connect");

			if (i > 0) {
				if (addr == NULL) {
					I_Error("Unable to resolve '%s'\n", myargv[i + 1]);
				}
			}
		}

		if (addr != NULL) {
			if (M_CheckParm("-drone") > 0) {
				drone = true;
			}

			//!
			// @category net
			//
			// Run as the left screen in three screen mode.
			//

			if (M_CheckParm("-left") > 0) {
				viewangleoffset = ANG90;
				drone = true;
			}

			//!
			// @category net
			//
			// Run as the right screen in three screen mode.
			//

			if (M_CheckParm("-right") > 0) {
				viewangleoffset = ANG270;
				drone = true;
			}

			if (!NET_CL_Connect(addr)) {
				I_Error("D_CheckNetGame: Failed to connect to %s\n",
					NET_AddrToString(addr));
			}

			I_Printf("D_CheckNetGame: Connected to %s\n", NET_AddrToString(addr));

			D_NetWait();
		}
	}

#endif
}

//
// D_QuitNetGame
// Called before quitting to leave a net game
// without hanging the other players
//

void D_QuitNetGame(void) {
	if (debugfile) {
		fclose(debugfile);
	}

#ifdef FEATURE_MULTIPLAYER
	NET_SV_Shutdown();
	NET_CL_Disconnect();
#endif
}

//
// PlayersInGame
// Returns true if there are currently any players in the game.
//

boolean PlayersInGame(void) {
	int i;

	for (i = 0; i < MAXPLAYERS; ++i) {
		if (playeringame[i]) {
			return true;
		}
	}

	return false;
}

//
// GetLowTic
//

int GetLowTic(void) {
	int i;
	int lowtic;

	if (net_client_connected) {
		lowtic = INT_MAX;

		for (i = 0; i < MAXPLAYERS; ++i) {
			if (playeringame[i]) {
				if (nettics[i] < lowtic) {
					lowtic = nettics[i];
				}
			}
		}
	}
	else {
		lowtic = maketic;
	}

	return lowtic;
}
