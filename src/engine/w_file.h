// Emacs style mode select   -*- C -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 2005 Simon Howard
// Copyright(C) 2007-2012 Samuel Villarreal
// Copyright(C) 2025 Gibbon
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

#ifndef __W_FILE__
#define __W_FILE__

#include "doomtype.h"

// atsb: basically rewritten

typedef struct _wad_file_s wad_file_t;

typedef struct {
	wad_file_t* (*OpenFile)(char* path);
	void (*CloseFile)(wad_file_t* file);
	unsigned int(*Read)(wad_file_t* file, unsigned int offset,
		void* buffer, unsigned int buffer_len);
} wad_file_class_t;

struct _wad_file_s {
	wad_file_class_t* file_class;
	byte* mapped;
	unsigned int length;
};

wad_file_t* W_OpenFile(char* path);
void W_CloseFile(wad_file_t* wad);
unsigned int W_Read(wad_file_t* wad, unsigned int offset,
	void* buffer, unsigned int buffer_len);

char* W_FindIWAD(void);

#endif /*__W_FILE__*/
