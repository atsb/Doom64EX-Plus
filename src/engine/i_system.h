// Emacs style mode select   -*- C -*-
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

#ifndef __I_SYSTEM__
#define __I_SYSTEM__

#include <stdio.h>

#include "d_ticcmd.h"
#include "d_event.h"
#include "m_fixed.h"

#ifdef __GNUG__
#pragma interface
#endif

// Called by DoomMain.
void I_Init(void);

// Called by D_DoomLoop,
// returns current time in tics.

extern fixed_t rendertic_frac;

extern int (*I_GetTime)(void);
void            I_InitClockRate(void);
int             I_GetTimeMS(void);
void            I_Sleep(unsigned long usecs);
boolean        I_StartDisplay(void);
void            I_EndDisplay(void);
fixed_t         I_GetTimeFrac(void);
void            I_GetTime_SaveMS(void);
unsigned long   I_GetRandomTimeSeed(void);

// Asynchronous interrupt functions should maintain private queues
// that are read by the synchronous functions
// to be converted into events.

// Either returns a null ticcmd,
// or calls a loadable driver to build it.
// This ticcmd will then be modified by the gameloop
// for normal input.
ticcmd_t* I_BaseTiccmd(void);

// Called by M_Responder when quit is selected.
// Clean exit, displays sell blurb.
void I_Quit(void);

void I_BeginRead(void);
void I_Error(const char* error, ...);
void I_Warning(const char* string, ...);
void I_Printf(const char* msg, ...);

char* I_GetUserDir(void);
char* I_GetUserFile(char* file);
char* I_FindDataFile(char* file);

boolean I_FileExists(const char* path);

void I_RegisterCvars(void);

extern FILE* DebugFile;
extern boolean    DigiJoy;

#ifdef _MSC_VER
#include <direct.h>
#include <io.h>
#define F_OK 0
#define W_OK 2
#define R_OK 4
#define S_ISDIR(x) (((sbuf.st_mode & S_IFDIR)==S_IFDIR)?1:0)
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#endif

#endif
