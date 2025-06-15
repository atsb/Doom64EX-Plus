// Emacs style mode select   -*- C -*-
//-----------------------------------------------------------------------------
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
// Network client code
//
//------------------------------------------------------------------------------

#ifndef NET_CLIENT_H
#define NET_CLIENT_H

#include "doomdef.h"
#include "doomtype.h"
#include "d_ticcmd.h"
#include "md5.h"
#include "net_defs.h"

#define MAXPLAYERNAME 30

boolean NET_CL_Connect(net_addr_t* addr);
void NET_CL_Disconnect(void);
void NET_CL_Run(void);
void NET_CL_Init(void);
void NET_CL_StartGame(void);
void NET_CL_SendTiccmd(ticcmd_t* ticcmd, int maketic);
void NET_Init(void);
void NET_CL_SendCheat(int player, int type, char* buff);

extern boolean net_client_connected;
extern boolean net_client_received_wait_data;
extern boolean net_client_controller;
extern unsigned int net_clients_in_game;
extern unsigned int net_drones_in_game;
extern boolean net_waiting_for_start;
extern char net_player_names[MAXPLAYERS][MAXPLAYERNAME];
extern char net_player_addresses[MAXPLAYERS][MAXPLAYERNAME];
extern int net_player_number;
extern char* net_player_name;

extern md5_digest_t net_server_wad_md5sum;
extern md5_digest_t net_local_wad_md5sum;

#endif /* #ifndef NET_CLIENT_H */
