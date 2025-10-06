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


#ifndef __P_SAVEG__
#define __P_SAVEG__

#include "doomtype.h"

#define SAVEGAMESIZE    0x60000
#define SAVEGAMETBSIZE  0xC000
#define SAVESTRINGSIZE  16

char* P_GetSaveGameName(int num);
boolean P_WriteSaveGame(char* description, int slot);
boolean P_ReadSaveGame(char* name);
boolean P_QuickReadSaveHeader(char* name, char* date, int* thumbnail, int* skill, int* map);

// Persistent storage/archiving.
// These are the load / save game routines.
void P_ArchivePlayers(void);
void P_UnArchivePlayers(void);
void P_ArchiveWorld(void);
void P_UnArchiveWorld(void);
void P_ArchiveMobjs(void);
void P_UnArchiveMobjs(void);
void P_ArchiveSpecials(void);
void P_UnArchiveSpecials(void);
void P_ArchiveMacros(void);
void P_UnArchiveMacros(void);

#endif
