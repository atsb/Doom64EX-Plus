// Emacs style mode select   -*- C -*-
//-----------------------------------------------------------------------------
//
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

#ifndef __W_FILE__
#define __W_FILE__

#include <stdio.h>
#include "doomtype.h"

typedef struct _wad_file_s wad_file_t;

typedef struct {
	// Open a file for reading.

	wad_file_t* (*OpenFile)(char* path);

	// Close the specified file.

	void (*CloseFile)(wad_file_t* file);

	// Read data from the specified position in the file into the
	// provided buffer.  Returns the number of bytes read.

	unsigned int(*Read)(wad_file_t* file, unsigned int offset,
		void* buffer, unsigned int buffer_len);
} wad_file_class_t;

struct _wad_file_s {
	// Class of this file.

	wad_file_class_t* file_class;

	// If this is NULL, the file cannot be mapped into memory.  If this
	// is non-NULL, it is a pointer to the mapped file.

	byte* mapped;

	// Length of the file, in bytes.

	unsigned int length;
};

// Open the specified file. Returns a pointer to a new wad_file_t
// handle for the WAD file, or NULL if it could not be opened.

wad_file_t* W_OpenFile(char* path);

// Close the specified WAD file.

void W_CloseFile(wad_file_t* wad);

// Read data from the specified file into the provided buffer.  The
// data is read from the specified offset from the start of the file.
// Returns the number of bytes read.

unsigned int W_Read(wad_file_t* wad, unsigned int offset,
	void* buffer, unsigned int buffer_len);

char* W_FindWADByName(char* filename);
char* W_TryFindWADByName(char* filename);
char* W_FindIWAD(void);

#endif /*__W_FILE__*/
