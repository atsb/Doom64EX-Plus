// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 1993-1997 Id Software, Inc.
// Copyright(C) 2007-2012 Samuel Villarreal
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
// 02111-1307, USA.
//
//-----------------------------------------------------------------------------
//
// DESCRIPTION: System Interface
//
//-----------------------------------------------------------------------------

#include <stdlib.h>
#include <stdio.h>

#ifdef _WIN32
#include <direct.h>
#include <io.h>
#endif

#include <stdarg.h>
#include <sys/stat.h>
#include "doomstat.h"
#include "doomdef.h"
#include "m_misc.h"
#include "i_video.h"
#include "d_net.h"
#include "g_demo.h"
#include "d_main.h"
#include "con_console.h"
#include "z_zone.h"
#include "i_system.h"
#include "i_audio.h"
#include "gl_draw.h"

#ifdef _WIN32
#include "i_xinput.h"
#endif

CVAR(i_interpolateframes, 1);
CVAR(v_vsync, 1);
CVAR(v_accessibility, 0);

// Gibbon - hack from curl to deal with some crap
#if !defined(S_ISREG) && defined(S_IFMT) && defined(S_IFREG)
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#endif

ticcmd_t        emptycmd;
static int64    I_GetTime_Scale = 1 << 24;

//
// I_uSleep
//

void I_Sleep(uint32_t usecs) {
	SDL_Delay(usecs);
}

static int basetime = 0;

//
// I_GetTimeNormal
//

static int I_GetTimeNormal(void) {
	uint32 ticks;

	ticks = SDL_GetTicks();

	if (basetime == 0) {
		basetime = ticks;
	}

	ticks -= basetime;

	return (ticks * TICRATE) / 1000;
}

//
// I_GetTime_Scaled
//

static int I_GetTime_Scaled(void) {
	return (int)((int64)I_GetTimeNormal() * I_GetTime_Scale >> 24);
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

static uint32_t start_displaytime;
static uint32_t displaytime;
static dboolean InDisplay = false;
static int saved_gametic = -1;

dboolean realframe = false;

fixed_t         rendertic_frac = 0;
uint32_t    rendertic_start;
uint32_t    rendertic_step;
uint32_t    rendertic_next;
const float     rendertic_msec = 100 * TICRATE / 100000.0f;

//
// I_StartDisplay
//

dboolean I_StartDisplay(void) {
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
	uint32_t now;
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
// I_GetTime_SaveMS
//

void I_GetTime_SaveMS(void) {
	rendertic_start = SDL_GetTicks();
	rendertic_next = (uint32_t)((rendertic_start * rendertic_msec + 1.0f) / rendertic_msec);
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
 * @note The returning value MUST be freed by the caller.
 */

int8_t* I_GetUserDir(void) 
{
	return SDL_GetBasePath();
}

/**
 * @brief Get the directory which contains this program.
 *
 * @return Fully-qualified path that ends with a separator or NULL if not 

/**
 * @brief Find a regular file in the user-writeable directory.
 *
 * @return Fully-qualified path or NULL if not found.
 * @note The returning value MUST be freed by the caller.
 */

 /* ATSB: for some reason, these free() calls cause MSVC builds to bail out
 *  but are needed for Unix systems to even boot the game.
 *  whatever...  eventually we will clean up this mess and have
 *  portable fixed width types everywhere...  one day.
 *  WOLF3S 5-11-2022: Changed to SDL_free for some underterminated time!
 */
int8_t* I_GetUserFile(const int8_t* file) {
	uint8_t* path, * userdir;

	if (!(userdir = I_GetUserDir()))
		return NULL;

	path = (uint8_t*)malloc(512);

	snprintf(path, 511, "%s%s", userdir, file);


#if defined(__LINUX__) || defined(__OpenBSD__)
	free(userdir);
#else
	SDL_free(userdir);
#endif
	return path;
}

/**
 * @brief Find a regular read-only data file.
 *
 * @return Fully-qualified path or NULL if not found.
 * @note The returning value MUST be freed by the caller.
 */
int8_t* I_FindDataFile(const int8_t* file) {
	uint8_t* path, * dir;

	path = (uint8_t*)malloc(512);

	if ((dir = I_GetUserDir())) {
		snprintf(path, 511, "%s%s", dir, file);

#if defined(__LINUX__) || defined(__OpenBSD__)
		free(dir);
#else
		SDL_free(dir);
#endif
		if (I_FileExists(path))
			return path;
	}

	if ((dir = I_GetUserDir())) {
		snprintf(path, 511, "%s%s", dir, file);

#if defined(__LINUX__) || defined(__OpenBSD__)
		free(dir);
#else
		SDL_free(dir);
#endif

		if (I_FileExists(path))
			return path;
	}

#if defined(__LINUX__) || defined(__OpenBSD__)
	{
		int i;
		const int8_t* paths[] = {
			//André: Removed all useless directories, Only The dir usr/local is fine to use.
				//"/usr/local/share/games/doom64ex-plus/",
				"/usr/local/share/doom64ex-plus/",
				//"/usr/local/share/doom/",
				//"/usr/share/games/doom64ex-plus/",
				//"/usr/share/doom64ex-plus/",
				//"/usr/share/doom/",
				//"/opt/doom64ex-plus/",
		};

		for (i = 0; i < sizeof(paths) / sizeof(*paths); i++) {
			snprintf(path, 511, "%s%s", paths[i], file);
			if (I_FileExists(path))
				return path;
		}
	}
#endif

#if defined(__LINUX__) || defined(__OpenBSD__)
	free(path);
#else
	SDL_free(path);
#endif
	

	return NULL;
}

/**
 * @brief Checks to see if the given absolute path is a regular file.
 * @param path Absolute path to check.
 */

dboolean I_FileExists(const int8_t* path)
{
	struct stat st;
	return !stat(path, &st) && S_ISREG(st.st_mode);
}

//
// I_GetTime
// returns time in 1/70th second tics
// now 1/35th?
//

int (*I_GetTime)(void) = I_GetTime_Error;

//
// I_GetTimeMS
//
// Same as I_GetTime, but returns time in milliseconds
//

int I_GetTimeMS(void) {
	uint32 ticks;

	ticks = SDL_GetTicks();

	if (basetime == 0) {
		basetime = ticks;
	}

	return ticks - basetime;
}

//
// I_GetRandomTimeSeed
//

uint32_t I_GetRandomTimeSeed(void) {
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

void I_Error(const int8_t* string, ...) {
	int8_t buff[1024];
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

void I_Printf(const int8_t* string, ...) {
	int8_t buff[1024];
	va_list    va;

	dmemset(buff, 0, 1024);

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

dboolean    inshowbusy = false;

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

#ifdef _USE_XINPUT
CVAR_EXTERNAL(i_rsticksensitivity);
CVAR_EXTERNAL(i_rstickthreshold);
CVAR_EXTERNAL(i_xinputscheme);
#endif

CVAR_EXTERNAL(i_gamma);
CVAR_EXTERNAL(i_brightness);
CVAR_EXTERNAL(v_vsync);
CVAR_EXTERNAL(v_accessibility);

void I_RegisterCvars(void) {
#ifdef _USE_XINPUT
	CON_CvarRegister(&i_rsticksensitivity);
	CON_CvarRegister(&i_rstickthreshold);
	CON_CvarRegister(&i_xinputscheme);
#endif

	CON_CvarRegister(&i_gamma);
	CON_CvarRegister(&i_brightness);
	CON_CvarRegister(&i_interpolateframes);
	CON_CvarRegister(&v_vsync);
	CON_CvarRegister(&v_accessibility);
}
