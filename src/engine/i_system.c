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
//
// DESCRIPTION: System Interface
//
//-----------------------------------------------------------------------------

#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_platform_defines.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/stat.h>

#include "i_system.h"
#include "i_system_io.h"
#include "doomstat.h"
#include "doomdef.h"
#include "m_misc.h"
#include "i_video.h"
#include "i_sdlinput.h"
#include "g_demo.h"
#include "d_main.h"
#include "con_console.h"
#include "con_cvar.h"
#include "gl_draw.h"
#include "steam.h"

extern void I_ShutdownSound(void);

CVAR(i_interpolateframes, 1);
CVAR(v_accessibility, 0);
CVAR(v_fadein, 1);

#ifdef SDL_PLATFORM_ANDROID
#define GetBasePath()   SDL_GetAndroidInternalStoragePath();
#elif defined(DOOM_UNIX_INSTALL)
#define GetBasePath()	SDL_GetPrefPath("", "doom64ex-plus"); // returns allocated string
#else 
#define GetBasePath()	(char *)SDL_GetBasePath(); // not guaranteed to be writeable !
#endif

ticcmd_t        emptycmd;

typedef struct {
	char* filename;
	char* path;
} datafile_t;

static datafile_t** g_cached_datafiles = NULL;
static int g_num_cached_datafiles = 0;

//
// I_Sleep
//
void I_Sleep(unsigned long ms) {
	SDL_Delay(ms);
}

static Uint64 basetime = 0;

//
// I_GetTimeNormal
//

static int I_GetTimeNormal(void) {
	Uint64 ticks;
	Uint64 tic_division = 1000;

	ticks = SDL_GetTicks();

	if (basetime == 0) {
		basetime = ticks;
	}

	ticks -= basetime;

	return (ticks * TICRATE) / tic_division;
}

//
// I_GetTime_Error
//

static int I_GetTime_Error(void) {
	I_Error("I_GetTime_Error: GetTime() used before initialization");
	return 0;
}

//
// I_InitClockRate
//

void I_InitClockRate(void) {
	I_GetTime = I_GetTimeNormal;
}

//
// FRAME INTERPOLTATION
//

static Uint64 start_displaytime;
static Uint64 displaytime;
static boolean InDisplay = false;

boolean realframe = false;

fixed_t         rendertic_frac = 0;
Uint64			rendertic_start;
Uint64			rendertic_step;
Uint64			rendertic_next;
const float     rendertic_msec = 100 * TICRATE / 100000.0f;

//
// I_StartDisplay
//

boolean I_StartDisplay(void) {
	rendertic_frac = I_GetTimeFrac();

	if (InDisplay) {
		return false;
	}

	start_displaytime = SDL_GetTicks();
	InDisplay = true;

	return true;
}

//
// I_EndDisplay
//

void I_EndDisplay(void) {
	displaytime = SDL_GetTicks() - start_displaytime;
	InDisplay = false;
}

//
// I_GetTimeFrac
//

fixed_t I_GetTimeFrac(void) {
	Uint64 now;
	fixed_t frac;

	now = SDL_GetTicks();

	if (rendertic_step == 0) {
		return FRACUNIT;
	}
	else {
		frac = (fixed_t)((now - rendertic_start + displaytime) * FRACUNIT / rendertic_step);
		if (frac < 0) {
			frac = 0;
		}
		if (frac > FRACUNIT) {
			frac = FRACUNIT;
		}
		return frac;
	}
}

//
// I_GetTimeMS
//
// Same as I_GetTime, but returns time in milliseconds
//

int I_GetTimeMS(void) {
	Uint64 ticks;

	ticks = SDL_GetTicks();

	if (basetime == 0) {
		basetime = ticks;
	}

	return ticks - basetime;
}

//
// I_GetTime_SaveMS
//

void I_GetTime_SaveMS(void) {
	rendertic_start = SDL_GetTicks();
	rendertic_next = (unsigned int)((rendertic_start * rendertic_msec + 1.0f) / rendertic_msec);
	rendertic_step = rendertic_next - rendertic_start;
}

//
// I_BaseTiccmd
//

ticcmd_t* I_BaseTiccmd(void) {
	return &emptycmd;
}

/**
 * @brief Get the user-writeable directory.
 *
 * Assume this is the only user-writeable directory on the system.
 *
 * @return Fully-qualified path that ends with a separator or NULL if not found.
 */

char* I_GetUserDir(void) 
{
	return GetBasePath();
}

/*
 * @brief Get the directory which contains this program.
 *
 * @return Fully-qualified path that ends with a separator or NULL if not 

 * @brief Find a regular file in the user-writeable directory.
 *
 * @return Fully-qualified path or NULL if not found.
 */

char* I_GetUserFile(char* file) {
	char* path, * userdir;

	if (!(userdir = I_GetUserDir()))
		return NULL;

	path = malloc(MAX_PATH);

	SDL_snprintf(path, MAX_PATH, "%s%s", userdir, file);

#ifdef DOOM_UNIX_INSTALL
    // SDL_GetPrefPath() returns an allocated string
    SDL_free(userdir);
#endif

    return path;
}


/**
 * @brief Find a regular read-only data file.
 *
 * @return Fully-qualified path or NULL if not found.
 */


static char* FindDataFile(char* file) {
	char *path = malloc(MAX_PATH);
	const char* dir;
	steamgame_t game;
	static boolean steam_install_dir_found = false;
	static filepath_t steam_install_dir;
    
	if (path == NULL) {
		return NULL;
	}

	if ((dir = SDL_GetBasePath())) {
        SDL_snprintf(path, MAX_PATH, "%s%s", dir, file);
		if (I_FileExists(path)) {
			return path;
		}
	}

#ifndef SDL_PLATFORM_WIN32

#ifdef DOOM_UNIX_INSTALL
	if ((dir = I_GetUserDir())) {
		SDL_snprintf(path, MAX_PATH, "%s%s", dir, file);
		if (I_FileExists(path)) {
			return path;
		}
	}
#endif    
    
#ifdef DOOM_UNIX_SYSTEM_DATADIR
	SDL_snprintf(path, MAX_PATH, "%s/%s", DOOM_UNIX_SYSTEM_DATADIR, file);
	if (I_FileExists(path)) {
		return path;
	}
#endif

	SDL_snprintf(path, MAX_PATH, "%s", file);
	if (I_FileExists(path)) {
		return path;
	}
#else // SDL_PLATFORM_WIN32

	// detect GOG prior to Steam because it is faster

	filepath_t install_dir;
	if (I_GetRegistryString(HKEY_LOCAL_MACHINE,
		L"SOFTWARE\\Wow6432Node\\GOG.com\\Games\\1456611261", L"path", install_dir, MAX_PATH)) {
		SDL_snprintf(path, MAX_PATH, "%s/%s", install_dir, file);
		if (I_FileExists(path)) {
			I_Printf("I_FindDataFile: Adding GOG file %s\n", path);
			return path;
		}
	}

#endif

#if defined(SDL_PLATFORM_WIN32) || defined(SDL_PLATFORM_LINUX)  

    // cache Steam install dir value as calling Steam_FindGame is a bit expensive

	if(!steam_install_dir_found) {
        steam_install_dir_found = Steam_FindGame(&game, DOOM64_STEAM_APPID)
            && Steam_ResolvePath(steam_install_dir, &game);
    }

    if(steam_install_dir_found) {
       SDL_snprintf(path, MAX_PATH, "%s/%s", steam_install_dir, file); 
	   if (I_FileExists(path)) {
		   I_Printf("I_FindDataFile: Adding Steam file %s\n", path);
		   return path;
	   }
    }
    
#endif
	free(path);
	return NULL;
}

// returned string is cached and must NOT be freed by caller
char* I_FindDataFile(char* file) {

	datafile_t* entry = NULL;

	for (int i = 0; i < g_num_cached_datafiles; i++) {
		if (!dstrcmp(file, g_cached_datafiles[i]->filename)) {
			entry = g_cached_datafiles[i];
			break;
		}
	}

	if (!entry) {
		entry = malloc(sizeof(datafile_t));
		entry->filename = M_StringDuplicate(file);
		entry->path = FindDataFile(file);

		g_cached_datafiles = realloc(g_cached_datafiles, (g_num_cached_datafiles + 1) * sizeof(datafile_t *));
		g_cached_datafiles[g_num_cached_datafiles] = entry;
		g_num_cached_datafiles++;
	}

	return entry->path; // may be NULL
}


/**
 * @brief Checks to see if the given absolute path is a regular file.
 * @param path Absolute path to check.
 */

boolean I_FileExists(const char* path)
{
	struct stat st;
	return path && !stat(path, &st) && S_ISREG(st.st_mode);
}

boolean I_DirExists(const char* path)
{
	struct stat st;
	return path && !stat(path, &st) && S_ISDIR(st.st_mode);
}

//
// I_GetTime
// returns time in 1/70th second tics
// now 1/35th?
//

int (*I_GetTime)(void) = I_GetTime_Error;

//
// I_GetRandomTimeSeed
//

unsigned long I_GetRandomTimeSeed(void) {
	// not exactly random....
	return SDL_GetTicks();
}

//
// I_Init
//

void I_Init(void)
{
	I_InitVideo();
	I_InitClockRate();
}

//
// I_Error
//

void I_Error(const char* string, ...) {
	char buff[1024];
	va_list    va;

	I_ShutdownSound();

	va_start(va, string);
	vsprintf(buff, string, va);
	va_end(va);

	fprintf(stderr, "Error - %s\n", buff);
	fflush(stderr);

	I_Printf("\n********* ERROR *********\n");
	I_Printf("%s", buff);

	if (usingGL) {
		while (1) {
			GL_ClearView(0xFF000000);
			Draw_Text(0, 0, WHITE, 1, 1, "Error - %s\n", buff);
			GL_SwapBuffers();

			if (I_ShutdownWait()) {
				break;
			}

			I_Sleep(1);
		}
	}
	else {
		I_ShutdownVideo();
	}
	exit(0);    // just in case...
}

void I_Warning(const char* string, ...) {
	char buff[1024];
	va_list    va;

	va_start(va, string);
	vsprintf(buff, string, va);
	va_end(va);

	fprintf(stderr, "Warning - %s\n", buff);
	fflush(stderr);

	I_Printf("\n********* WARNING *********\n");
	I_Printf("%s", buff);

	if (usingGL) {
		while (1) {
			GL_ClearView(0xFF000000);
			Draw_Text(0, 0, WHITE, 1, 1, "Warning - %s\n", buff);
			GL_SwapBuffers();

			I_Sleep(1);
		}
	}
}

//
// I_Quit
//

void I_Quit(void) {
	if (demorecording) {
		endDemo = true;
		G_CheckDemoStatus();
	}
	M_SaveDefaults();
	I_ShutdownSound();
	I_ShutdownVideo();

	exit(0);
}

//
// I_Printf
//

void I_Printf(const char* string, ...) {
	char buff[1024];
	va_list    va;

	dmemset(buff, 0, sizeof(buff));

	va_start(va, string);
	vsprintf(buff, string, va);
	va_end(va);
	printf("%s", buff);
	if (console_initialized) {
		CON_AddText(buff);
	}
}

//
// I_BeginRead
//

boolean    inshowbusy = false;

void I_BeginRead(void) {
	if (!devparm) {
		return;
	}

	if (inshowbusy) {
		return;
	}
	inshowbusy = true;
	inshowbusy = false;
	BusyDisk = true;
}

//
// I_RegisterCvars
//

CVAR_EXTERNAL(i_gamma);
CVAR_EXTERNAL(i_brightness);
CVAR_EXTERNAL(v_accessibility);
CVAR_EXTERNAL(v_fadein);

void I_RegisterCvars(void) {
	CON_CvarRegister(&i_gamma);
	CON_CvarRegister(&i_brightness);
	CON_CvarRegister(&i_interpolateframes);
	CON_CvarRegister(&v_accessibility);
	CON_CvarRegister(&v_fadein);
}


#ifdef SDL_PLATFORM_WIN32

boolean I_GetRegistryString (HKEY root, const wchar_t *dir, const wchar_t *keyname, char *out, size_t maxchars)
{
	LSTATUS		err;
	HKEY		key;
	WCHAR		wpath[MAX_PATH + 1];
	DWORD		size, type;

	if (!maxchars)
		return false;
	*out = 0;

	err = RegOpenKeyExW (root, dir, 0, KEY_READ, &key);
	if (err != ERROR_SUCCESS)
		return false;

	// Note: string might not contain a terminating null character
	// https://docs.microsoft.com/en-us/windows/win32/api/winreg/nf-winreg-regqueryvalueexw#remarks

	err = RegQueryValueExW (key, keyname, NULL, &type, NULL, &size);
	if (err != ERROR_SUCCESS || type != REG_SZ || size > sizeof (wpath) - sizeof (wpath[0]))
	{
		RegCloseKey (key);
		return false;
	}

	err = RegQueryValueExW (key, keyname, NULL, &type, (BYTE *)wpath, &size);
	RegCloseKey (key);
	if (err != ERROR_SUCCESS || type != REG_SZ)
		return false;

	wpath[size / sizeof (wpath[0])] = 0;

	if (WideCharToMultiByte (CP_UTF8, 0, wpath, -1, out, maxchars, NULL, NULL) != 0)
		return true;
	*out = 0;
	return false;
}

#endif
