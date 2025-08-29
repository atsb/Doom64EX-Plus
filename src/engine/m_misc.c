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
//
// DESCRIPTION:
//      Main loop menu stuff.
//      Default Config File.
//        Executable arguments
//        BBox stuff
//
//-----------------------------------------------------------------------------

#include <stdbool.h>
#include <limits.h>
#include <stdlib.h>
#include <errno.h>
#include <SDL3/SDL_platform_defines.h>
#include <SDL3/SDL_stdinc.h>
#include <fcntl.h>

// for mkdir/_mkdir
#ifdef SDL_PLATFORM_WIN32
#include <direct.h> 
#else 
#include <sys/stat.h>
#endif

#include "m_misc.h"
#include "doomstat.h"
#include "z_zone.h"
#include "i_png.h"
#include "i_system.h"
#include "p_saveg.h"
#include "g_actions.h"
#include "g_settings.h"
#include "gl_main.h"
#include "doomdef.h"
#include "i_system_io.h"


int      myargc;
char**   myargv;

//
// M_CheckParm
// Checks for the given parameter
// in the program's command line arguments.
// Returns the argument number (1 to argc-1)
// or 0 if not present

int M_CheckParm(const char* check) {
	int        i;

	for (i = 1; i < myargc; i++) {
		if (!dstricmp(check, myargv[i])) { //strcasecmp
			return i;
		}
	}

	return 0;
}

char* M_StringDuplicate(char* s)
{
	return strdup(s);
}

//
// M_ClearBox
//

void M_ClearBox(fixed_t* box) {
	box[BOXTOP] = box[BOXRIGHT] = INT_MIN;
	box[BOXBOTTOM] = box[BOXLEFT] = INT_MAX;
}

//
// M_AddToBox
//

void M_AddToBox(fixed_t* box, fixed_t x, fixed_t y) {
	if (x < box[BOXLEFT]) {
		box[BOXLEFT] = x;
	}
	else if (x > box[BOXRIGHT]) {
		box[BOXRIGHT] = x;
	}
	if (y < box[BOXBOTTOM]) {
		box[BOXBOTTOM] = y;
	}
	else if (y > box[BOXTOP]) {
		box[BOXTOP] = y;
	}
}

//
// M_WriteFile
//

boolean M_WriteFile(char* filepath, void* source, int length) {
	FILE* fp;
	boolean result;

	if (!(fp = fopen(filepath, "wb"))) {
		return 0;
	}

	I_BeginRead();
	result = (fwrite(source, 1, length, fp) == length);
	fclose(fp);

	if (!result) {
		M_RemoveFile(filepath);
	}

	return result;
}

//
// M_WriteTextFile
//

boolean M_WriteTextFile(char* filepath, char* source, int length) {
	int handle;
	int count;
	handle = open(filepath, O_WRONLY | O_CREAT | O_TRUNC, 0666);

	if (handle == -1) {
		return false;
	}
	count = write(handle, source, length);
	close(handle);

	if (count < length) {
		return false;
	}

	return true;
}

//
// M_ReadFile
//


int M_ReadFileEx(char* filepath, byte** buffer, boolean use_malloc) {
	FILE* fp;
	int length = M_FileLengthFromPath(filepath);

	*buffer = NULL;

	if (length == -1) return -1;

	fp = fopen(filepath, "rb");
	if (!fp) return -1;

	*buffer = use_malloc ? malloc(length) : Z_Malloc(length, PU_STATIC, 0);
	if (*buffer) {

		I_BeginRead();

		if (fread(*buffer, 1, length, fp) != length) {
			length = -1;

			if (use_malloc) {
				free(*buffer);
			}
			else {
				Z_Free(*buffer);
			}
			*buffer = NULL;
		}
	}
	else {
		length = -1;
	}

	fclose(fp);

	return length;
}

// move filename in src_dirpath to dst_dirpath
// return false if file already exists or other error, true on success
boolean M_MoveFile(char* filename, char* src_dirpath, char* dst_dirpath) {

	filepath_t src_filepath, dst_filepath;
	byte* data;
	int data_len;
	boolean ret;

	SDL_snprintf(dst_filepath, MAX_PATH, "%s%s", dst_dirpath, filename);

	if (I_FileExists(dst_filepath)) {
		return false;
	}

	SDL_snprintf(src_filepath, MAX_PATH, "%s%s", src_dirpath, filename);

	if ((data_len = M_ReadFileEx(src_filepath, &data, true)) == -1) {
		return false;
	}
	
	if (ret = M_WriteFile(dst_filepath, data, data_len)) {
		M_RemoveFile(src_filepath);
	}

	free(data);

	return ret;

}


int M_ReadFile(char* name, byte** buffer) {
	return M_ReadFileEx(name, buffer, false);
}

long M_FileLengthFromPath(char* filepath) {
	struct stat st;
	return stat(filepath, &st) == 0 ? st.st_size : -1;
}

//
// M_FileLength
//

boolean M_RemoveFile(char* filepath) {
	return remove(filepath) == 0;
}

long M_FileLength(FILE* handle) {
	long savedpos;
	long length;

	// save the current position in the file
	savedpos = ftell(handle);

	// jump to the end and find the length
	fseek(handle, 0, SEEK_END);
	length = ftell(handle);

	// go back to the old location
	fseek(handle, savedpos, SEEK_SET);

	return length;
}


boolean M_CreateDir(char* dirpath) {
	int ret;
#ifdef SDL_PLATFORM_WIN32
	ret = _mkdir(dirpath);
#else 
	ret = mkdir(dirpath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#endif
	return !ret;
}

//
// W_FileExists
// Check if a wad file exists
//

int M_FileExists(char* filepath) {
	FILE* fstream;

	fstream = fopen(filepath, "r");

	if (fstream != NULL) {
		fclose(fstream);
		return 1;
	}
	else {
		// If we can't open because the file is a directory, the
		// "file" exists at least!

		if (errno == 21) {
			return 2;
		}
	}

	return 0;
}

// Returns full path to filename if filename exists within dir
// Returns NULL if dirpath is NULL or empty string
// Must be freed by caller
char* M_FileExistsInDirectory(char* dirpath, char* filename, boolean log) {

	filepath_t path;

	if (!dstrisempty(dirpath)) {
		// not strictly necessary but avoid funky looking paths in the console
		char last_char = dirpath[dstrlen(dirpath) - 1];
		char* separator = last_char == '/' || last_char == '\\' ? "" : "/";

		SDL_snprintf(path, MAX_PATH, "%s%s%s", dirpath, separator, filename);
		if (I_FileExists(path)) {
			if (log) {
				I_Printf("Found %s\n", path);
			}
			return M_StringDuplicate(path);
		}
	}

	return NULL;
}


//
// M_SaveDefaults
//

void M_SaveDefaults(void) {
	FILE* fh;

	fh = fopen(G_GetConfigFileName(), "wt");
	if (fh) {
        G_OutputBindings(fh);
		fclose(fh);
	}
}

//
// M_LoadDefaults
//

void M_LoadDefaults(void) {
	G_LoadSettings();
}

//
// M_ScreenShot
//

void M_ScreenShot(void) {
	filepath_t name;
	byte* buff;
	byte* png;
	int size = 0;

	for (int i = 0; ; i++) {
		if (i == 1000) return;
		SDL_snprintf(name, MAX_PATH, "%ssshot%03d.png", I_GetUserDir(), i);
		if (!I_FileExists(name)) break;
	}

	buff = GL_GetScreenBuffer(0, 0, video_width, video_height);
	png = I_PNGCreate(video_width, video_height, buff, &size); // buff is freed by I_PNGCreate

	if (png && M_WriteFile(name, png, size)) {
		I_Printf("Saved screenshot: %s\n", name);
	} else {
		I_Printf("Failed to create screenshot\n");
	}

	Z_Free(png);
	
}

// Safe string copy function that works like OpenBSD's strlcpy().
// Returns true if the string was not truncated.

bool M_StringCopy(char* dest, char* src, unsigned int dest_size)
{
	size_t len;

	if (dest_size >= 1)
	{
		dest[dest_size - 1] = '\0';
		strncpy(dest, src, dest_size - 1);
	}
	else
	{
		return false;
	}

	len = strlen(dest);
	return src[len] == '\0';
}

//
// M_CacheThumbNail
// Thumbnails are assumed they are
// uncompressed 128x128 RGB textures
//

int M_CacheThumbNail(byte** data) {
	byte* buff;
	byte* tbn;

	buff = GL_GetScreenBuffer(0, 0, video_width, video_height);
	tbn = Z_Calloc(SAVEGAMETBSIZE, PU_STATIC, 0);

	Z_Free(buff);

	*data = tbn;
	return SAVEGAMETBSIZE;
}
