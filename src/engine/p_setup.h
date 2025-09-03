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

#ifndef __P_SETUP__
#define __P_SETUP__

#include "doomstat.h"
#include "gl_main.h"

// NOT called by W_Ticker. Fixme.
void P_SetupLevel(int map, int playermask, skill_t skill);

// Called by startup code.
void P_Init(void);

void P_RegisterCvars(void);

//
// [kex] mapinfo
//
mapdef_t* P_GetMapInfo(int map);
clusterdef_t* P_GetCluster(int map);
episodedef_t* P_GetEpisode(int episode);
int P_GetNumEpisodes(void);
void P_InitMapInfo(void);
void P_ListMaps(void);

// 
void LOC_RegisterCvars(void);

//
// [kex] sky definitions
//
typedef enum {
	SKF_CLOUD = 0x1,
	SKF_THUNDER = 0x2,
	SKF_FIRE = 0x4,
	SKF_BACKGROUND = 0x8,
	SKF_FADEBACK = 0x10,
	SKF_VOID = 0x20
} skyflags_e;

typedef struct {
	char        flat[9];
	int         flags;
	char        pic[9];
	char        backdrop[9];
	rcolor      fogcolor;
	rcolor      skycolor[3];
	int         fognear;
} skydef_t;

void P_SetupSky(void);
int P_GetNumSkies(void);

#endif
