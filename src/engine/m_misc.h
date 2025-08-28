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

#ifndef __M_MISC__
#define __M_MISC__

#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>

#include "doomtype.h"
#include "m_fixed.h"

//
// MISC
//

#ifndef O_BINARY
#define O_BINARY 0
#endif

extern  int    myargc;
extern  char** myargv;

// Returns the position of the given parameter
// in the arg list (0 if not found).
int M_CheckParm(const char* check);

// Bounding box coordinate storage.
enum {
	BOXTOP,
	BOXBOTTOM,
	BOXLEFT,
	BOXRIGHT
};    // bbox coordinates

// Bounding box functions.
void M_ClearBox(fixed_t* box);

void
M_AddToBox
(fixed_t* box,
	fixed_t    x,
	fixed_t    y);

boolean M_CreateDir(char* dirname);
boolean M_WriteFile(char const* name, void* source, int length);
int M_ReadFileEx(char const* name, byte** buffer, boolean use_malloc);
int M_ReadFile(char const* name, byte** buffer);
int M_FileExists(char* filename);
long M_FileLengthFromPath(char const* filepath);
long M_FileLength(FILE* handle);
boolean M_WriteTextFile(char const* name, char* source, int length);
void M_ScreenShot(void);
int M_CacheThumbNail(byte** data);
void M_LoadDefaults(void);
void M_SaveDefaults(void);
bool M_StringCopy(char* dest, const char* src, unsigned int dest_size);
char* M_StringDuplicate(const char* orig);

//
// DEFAULTS
//
extern int        DualMouse;

extern int      viewwidth;
extern int      viewheight;

extern char* chat_macros[];

//extern boolean HighSound;

#endif
