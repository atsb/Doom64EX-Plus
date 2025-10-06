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

#ifndef __W_WAD__
#define __W_WAD__

#include "w_file.h"

#define PHSENSW_LUMPNAME "PHSENSW"

//
// WADFILE I/O related stuff.
//
typedef struct {
	char        name[8];
	wad_file_t* wadfile;
	int         position;
	int         size;
	int         next;
	int         index;
	void* cache;
} lumpinfo_t;

extern lumpinfo_t* lumpinfo;
extern int numlumps;

void            W_Init(void);
wad_file_t* W_AddFile(char* filename);
unsigned int    W_HashLumpName(const char* str);
int             W_CheckNumForName(const char* name);
int             W_GetNumForName(const char* name);
int             W_LumpLength(int lump);
void            W_ReadLump(int lump, void* dest);
void* W_GetMapLump(int lump);
void            W_CacheMapLump(int map);
void            W_FreeMapLump(void);
int             W_MapLumpLength(int lump);
void* W_CacheLumpNum(int lump, int tag);
void* W_CacheLumpName(const char* name, int tag);
boolean W_LumpNameEq(lumpinfo_t* lump, const char* name);

boolean W_KPFLoadInner(const char* inner, unsigned char** data, int* size);



#endif
