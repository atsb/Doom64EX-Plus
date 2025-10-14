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
#include "i_audio.h"
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
#include "w_file.h"

extern void I_ShutdownSound(void);

CVAR(i_interpolateframes, 1);
CVAR(v_accessibility, 0);
CVAR(v_fadein, 1);

ticcmd_t        emptycmd;

//
// I_Sleep
//
void I_Sleep(unsigned long ms) {
	SDL_Delay(ms);
}

static Uint64 basetime = 0;

//
// I_GetTimeNormal: returns time elapsed in number of ticks: 1s -> TICRATE, 2s -> 2*TICRATE, ...
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
	if (i_interpolateframes.value) {
		displaytime = SDL_GetTicks() - start_displaytime;
	}
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

 
// return fully qualified non-NULL path that ends with a separator. Must not be freed by caller.
char* I_GetUserDir(void) 
{
	static char* g_user_dir = NULL;

	if (!g_user_dir) {
		g_user_dir = SDL_GetPrefPath("", "doom64ex-plus"); // string allocated by SDL
		if (g_user_dir) {
			I_Printf("User data dir: %s\n", g_user_dir);
		} else {
			I_Error("Failed to get user data dir\n");
		}
	}

	return g_user_dir;
}



// return a fully qualified non-NULL file path. Must be freed by caller
char* I_GetUserFile(char* filename) {
	filepath_t path;
	SDL_snprintf(path, MAX_PATH, "%s%s", I_GetUserDir(), filename);
	return M_StringDuplicate(path);
}

// return a fully qualified path or NULL if not found. Must be freed by caller
static char* FindDataFile(char* file) {
	char* path;
	steamgame_t game;
#ifdef SDL_PLATFORM_WIN32
	filepath_t gog_install_dir;
#endif

	static boolean steam_install_dir_found = false;
	static filepath_t steam_install_dir;

	char* dirs[] = {
		(char *)SDL_GetBasePath(), // install dir (where executable is located)
		".", // cur dir from where executable is launched
		I_GetUserDir(), 
#ifdef DOOM_UNIX_SYSTEM_DATADIR
		DOOM_UNIX_SYSTEM_DATADIR
#endif
	};

	// discard unsupported ROM DOOM64.WAD, by their size largely < 10 MB
	long min_size = dstreq(file, IWAD_FILENAME) ? 10*1024*1024 : 0;

	for (int i = 0; i < SDL_arraysize(dirs); i++) {
		if ((path = M_FileOrDirExistsInDirectory(dirs[i], file, true))) {
			if (min_size == 0 || M_FileLengthFromPath(path) > min_size)	return path;
			I_Printf("Discarding not usable IWAD: %s\n", path);
		}
	}

#ifdef SDL_PLATFORM_WIN32

	// detect GOG prior to Steam because it is faster

	if (I_GetRegistryString(HKEY_LOCAL_MACHINE,
		L"SOFTWARE\\Wow6432Node\\GOG.com\\Games\\1456611261", L"path", gog_install_dir, MAX_PATH)) {
		if ((path = M_FileOrDirExistsInDirectory(gog_install_dir, file, true))) return path;
	}

#endif

#if defined(SDL_PLATFORM_WIN32) || defined(SDL_PLATFORM_LINUX)  

    // cache Steam install dir value as calling Steam_FindGame is a bit expensive

	if(!steam_install_dir_found) {
        steam_install_dir_found = Steam_FindGame(&game, DOOM64_STEAM_APPID)
            && Steam_ResolvePath(steam_install_dir, &game);
    }

    if(steam_install_dir_found && (path = M_FileOrDirExistsInDirectory(steam_install_dir, file, true))) return path;
    
#endif

	return NULL;
}

// return a full qualified path that is cached, or NULL if not found. Must NOT be freed by caller
char* I_FindDataFile(char* file) {

	typedef struct {
		char* filename;
		char* path;
	} datafile_t;

	static datafile_t** cached_datafiles = NULL;
	static int num_cached_datafiles = 0;

	datafile_t* entry = NULL;

	for (int i = 0; i < num_cached_datafiles; i++) {
		if (dstreq(file, cached_datafiles[i]->filename)) {
			entry = cached_datafiles[i];
			break;
		}
	}

	if (!entry) {
		entry = malloc(sizeof(datafile_t));
		entry->filename = M_StringDuplicate(file);
		entry->path = FindDataFile(file);

		cached_datafiles = realloc(cached_datafiles, (num_cached_datafiles + 1) * sizeof(datafile_t *));
		cached_datafiles[num_cached_datafiles] = entry;
		num_cached_datafiles++;
	}

	return entry->path; 
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
	SDL_vsnprintf(buff, sizeof(buff), string, va);
	va_end(va);

	I_Printf("ERROR: %s", buff);

	I_ShutdownVideo();
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "ERROR", buff, NULL);
	exit(0);
}

void I_Warning(const char* string, ...) {
	char buff[1024];
	va_list    va;

	va_start(va, string);
	SDL_vsnprintf(buff, sizeof(buff), string, va);
	va_end(va);

	I_Printf("WARNING: %s", buff);

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

	va_start(va, string);
	SDL_vsnprintf(buff, sizeof(buff), string, va);
	va_end(va);
	printf("%s", buff);
	fflush(stdout);
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
CVAR_EXTERNAL(i_overbright);
CVAR_EXTERNAL(v_accessibility);
CVAR_EXTERNAL(v_fadein);

void I_RegisterCvars(void) {
	CON_CvarRegister(&i_gamma);
	CON_CvarRegister(&i_brightness);
	CON_CvarRegister(&i_overbright);
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
