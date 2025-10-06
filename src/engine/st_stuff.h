// Emacs style mode select   -*- C -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 1993-1997 Id Software, Inc.
// Copyright(C) 1997 Midway Home Entertainment, Inc
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

#ifndef __STSTUFF_H__
#define __STSTUFF_H__

#include "doomtype.h"
#include "d_event.h"
#include "p_mobj.h"
#include "net_client.h"
#include "gl_main.h"

//
// STATUS BAR
//

// Called by main loop.
boolean ST_Responder(event_t* ev);

// Called by main loop.
void ST_Ticker(void);
void ST_ClearMessage(void);

// Called when the console player is spawned on each level.
void ST_Start(void);

// Called by startup code.
void ST_Init(void);
void ST_AddChatMsg(char* msg, int player);
void ST_Notification(char* msg);
void ST_Drawer(void);
void ST_FlashingScreen(byte r, byte g, byte b, byte a);
char ST_DequeueChatChar(void);
void ST_DrawCrosshair(int x, int y, int slot, byte scalefactor, rcolor color);
void ST_UpdateFlash(void);
void ST_AddDamageMarker(mobj_t* target, mobj_t* source);
void ST_ClearDamageMarkers(void);
void ST_RegisterCvars(void);
void ST_DisplayPendingWeapon(void);

extern char player_names[MAXPLAYERS][MAXPLAYERNAME];
extern boolean st_chatOn;
extern int st_crosshairs;

#endif
