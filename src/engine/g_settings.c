// Emacs style mode select   -*- C -*-
//-----------------------------------------------------------------------------
// Copyright(C) 1993-1997 Id Software, Inc.
// Copyright(C) 1999-2000 Paul Brook
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
// DESCRIPTION: Doom3D's config file parsing
//
//-----------------------------------------------------------------------------

#include <stdlib.h>

#include <SDL3/SDL_storage.h>

#include "g_settings.h"
#include "z_zone.h"
#include "m_misc.h"
#include "i_system.h"
#include "g_actions.h"

char    DefaultConfig[] =
#include "defconfig.inc"    
;



//
// G_ExecuteMultipleCommands
//

#include "i_system_io.h"


// move any existing data file present in SDL_GetBasePath() to I_GetUserDir()
// that's necessary to migrate files from existing install prior or equal to v4.2.0.0 
// to the new location, especially on Windows where data files where stored in the install directory (SDL_GetBasePath())
static void MoveUserDataFiles() {
	char* src_dirpath = (char *)SDL_GetBasePath();
	if (!src_dirpath) return;

	SDL_Storage* src_storage = SDL_OpenFileStorage(src_dirpath);
	if (!src_storage) return;

	char* patterns[] = {
		"config.cfg",
		"sshot???.png",
		"doomsav*.dsg"
	};

	for (int i = 0; i < SDL_arraysize(patterns); i++) {
		int count = 0;
		char** matches = SDL_GlobStorageDirectory(src_storage, NULL, patterns[i], 0, &count);
		
		if (matches) {
			for (int i = 0; i < count; i++) {
				char* filename = matches[i];
				if (!M_MoveFile(filename, src_dirpath, I_GetUserDir())) {
					I_Printf("WARNING: failed to move %s to %s\n", filename, I_GetUserDir());
				}
			}
			SDL_free(matches);
		}
	}
		
	SDL_CloseStorage(src_storage);
}

// return fully qualified non-NULL config file name. Must NOT be freed by caller
char* G_GetConfigFileName(void) {
	static char* g_config_filename = NULL;

	if (!g_config_filename) {

		MoveUserDataFiles();

		g_config_filename = I_GetUserFile("config.cfg");
		I_Printf("Config file: %s\n", g_config_filename);
	}

	return g_config_filename;
}

void G_ExecuteMultipleCommands(char* data) {
	char* p;
	char* q;
	char    c;
	char    line[1024];

	p = data;
	c = *p;
	while (c) {
		q = line;
		c = *(p++);
		while (c && (c != '\n')) {
			if (c != '\r') {
				*(q++) = c;
			}
			c = *(p++);
		}
		*q = 0;
		if (line[0]) {
			G_ExecuteCommand(line);
		}
	}
}

//
// G_ExecuteFile
//

void G_ExecuteFile(char* name) {
	FILE* fh;
	char* buff;
	int        len;

	if (!name) {
		I_Error("G_ExecuteFile: No config name specified");
	}

	fh = fopen(name, "rb");

	if (!fh) {
		fh = fopen(name, "w");
		if (!fh) {
			I_Error("G_ExecuteFile: Unable to create %s", name);
		}

		fprintf(fh, "%s", DefaultConfig);
		fclose(fh);

		fh = fopen(name, "rb");

		if (!fh) {
			I_Error("G_ExecuteFile: Failed to read %s", name);
		}
	}

	fseek(fh, 0, SEEK_END);
	len = ftell(fh);
	fseek(fh, 0, SEEK_SET);
	buff = Z_Malloc(len + 1, PU_STATIC, NULL);
	fread(buff, 1, len, fh);
	buff[len] = 0;
	G_ExecuteMultipleCommands(buff);
	Z_Free(buff);
}

//
// G_LoadSettings
//

boolean g_in_load_settings = false;

void G_LoadSettings(void) {
	g_in_load_settings = true;
	G_ExecuteFile(G_GetConfigFileName());
	g_in_load_settings = false;
}
