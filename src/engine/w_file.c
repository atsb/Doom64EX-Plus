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
//
// DESCRIPTION: WAD file routines. Adapted from Chocolate Doom
//
//-----------------------------------------------------------------------------

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "w_file.h"
#include "doomtype.h"
#include "doomdef.h"
#include "i_system.h"
#include "z_zone.h"
#include "m_misc.h"


typedef struct {
	wad_file_t wad;
	FILE* fstream;
} std_wad_file_t;


// atsb: these are just overloads, we use the simpler ones and the then these take care of the rest.
extern wad_file_class_t stdwadfile;

static wad_file_t* W_WAD_OpenFile(char* path) {
	std_wad_file_t* result;
	FILE* fstream;

	fstream = fopen(path, "rb");

	if (fstream == NULL) {
		return NULL;
	}

	result = (std_wad_file_t*)Z_Malloc(sizeof(std_wad_file_t), PU_STATIC, 0);
	result->wad.file_class = &stdwadfile;
	result->wad.mapped = NULL;
	result->wad.length = M_FileLength(fstream);
	result->fstream = fstream;

	return &result->wad;
}

static void W_WAD_CloseFile(wad_file_t* wad) {
	std_wad_file_t* stdwad;
	stdwad = (std_wad_file_t*)wad;
	fclose(stdwad->fstream);
	Z_Free(stdwad);
}

unsigned int W_WAD_Read(wad_file_t* wad, unsigned int offset,
	void* buffer, unsigned int buffer_len) {
	std_wad_file_t* stdwad;
	unsigned int result;
	stdwad = (std_wad_file_t*)wad;
	fseek(stdwad->fstream, offset, SEEK_SET);
	result = fread(buffer, 1, buffer_len, stdwad->fstream);

	return result;
}

wad_file_class_t stdwadfile = {
	W_WAD_OpenFile,
	W_WAD_CloseFile,
	W_WAD_Read,
};

wad_file_t* W_OpenFile(char* path) {
	return stdwadfile.OpenFile(path);
}

void W_CloseFile(wad_file_t* wad) {
	wad->file_class->CloseFile(wad);
}

unsigned int W_Read(wad_file_t* wad, unsigned int offset,
	void* buffer, unsigned int buffer_len) {
	return wad->file_class->Read(wad, offset, buffer, buffer_len);
}


//
// W_FindIWAD
// Checks availability of IWAD files by name,
//

char* W_FindIWAD(void)
{
	return I_FindDataFile(IWAD_FILENAME);
}
