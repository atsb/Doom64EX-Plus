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

#ifndef __G_GAME__
#define __G_GAME__

#include "doomdef.h"
#include "d_event.h"

extern boolean sendpause;

//
// GAME
//

void G_Init(void);
void G_ReloadDefaults(void);
void G_SaveDefaults(void);
void G_DeathMatchSpawnPlayer(int playernum);
void G_InitNew(skill_t skill, int map);
void G_DeferedInitNew(skill_t skill, int map);
void G_LoadGame(const char* name);
void G_DoLoadGame(void);
void G_SaveGame(int slot, const char* description);
void G_DoSaveGame(void);
void G_CompleteLevel(void);
void G_ExitLevel(void);
void G_SecretExitLevel(int map);
void G_Ticker(void);
void G_ScreenShot(void);
void G_RunTitleMap(void);
void G_RunGame(void);
void G_RegisterCvars(void);

boolean G_Responder(event_t* ev);

extern const int title_map_num;

#endif
