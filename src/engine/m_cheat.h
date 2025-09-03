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

#ifndef __M_CHEAT__
#define __M_CHEAT__

#include "d_player.h"
#include "d_event.h"

//
// CHEAT SEQUENCE PACKAGE
//

void M_CheatProcess(player_t* plyr, event_t* ev);
void M_ParseNetCheat(int player, int type, char* buff);

void M_CheatGod(player_t* player, char dat[4]);
void M_CheatClip(player_t* player, char dat[4]);
void M_CheatKfa(player_t* player, char dat[4]);
void M_CheatGiveWeapon(player_t* player, char dat[4]);
void M_CheatArtifacts(player_t* player, char dat[4]);
void M_CheatBoyISuck(player_t* player, char dat[4]);
void M_CheatGiveKey(player_t* player, char dat[4]);
void M_CheatWarp(player_t* player, char dat[4]);


extern int        amCheating;

#endif
