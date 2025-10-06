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
#include <stdbool.h>

#include "doomtype.h"
#include "m_fixed.h"

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
void M_AddToBox(fixed_t* box, fixed_t x, fixed_t y);

// File functions
boolean M_CreateDir(char* dirpath);
boolean M_WriteFile(char* filepath, void* source, int length);
int M_ReadFileEx(char* filepath, byte** buffer, boolean use_malloc);
int M_ReadFile(char* filepath, byte** buffer);
boolean M_RemoveFile(char* filepath);
boolean M_FileExists(const char* path);
boolean M_DirExists(const char* path);
char* M_FileOrDirExistsInDirectory(char* dirpath, char* filename, boolean log);
long M_FileLengthFromPath(char* filepath);
long M_FileLength(FILE* handle);
boolean M_MoveFile(char* filename, char* src_dirpath, char* dst_dirpath);
boolean M_WriteTextFile(char* filepath, char* source, int length);

void M_ScreenShot(void);
int M_CacheThumbNail(byte** data);
void M_LoadDefaults(void);
void M_SaveDefaults(void);
char* M_StringDuplicate(char* orig);
unsigned int M_StringHash(char* str);

//
// DEFAULTS
//
extern int        DualMouse;

extern int      viewwidth;
extern int      viewheight;

extern char* chat_macros[];


#endif
