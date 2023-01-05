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

#ifndef __M_MISC__
#define __M_MISC__
#ifndef C89
#include <stdbool.h>
#endif
#include <stdarg.h>
#include "i_w3swrapper.h"
#include "doomtype.h"
#include "m_fixed.h"
#include "r_local.h"

//
// MISC
//

#ifndef O_BINARY
#define O_BINARY 0
#endif

extern  int    myargc;
extern  char** myargv;

// Returns the position of the given parameter
// in the arg list (0 if not found).
int M_CheckParm(const int8_t* check);

// Bounding box coordinate storage.
enum {
	BOXTOP,
	BOXBOTTOM,
	BOXLEFT,
	BOXRIGHT
};    // bbox coordinates

// Bounding box functions.
void M_ClearBox(fixed_t* box);

void
M_AddToBox
(fixed_t* box,
	fixed_t    x,
	fixed_t    y);

boolean M_WriteFile(int8_t const* name, void* source, int length);
int M_ReadFile(int8_t const* name, byte** buffer);
void M_NormalizeSlashes(int8_t* str);
int M_FileExists(int8_t* filename);
long M_FileLength(FILE* handle);
boolean M_WriteTextFile(int8_t const* name, int8_t* source, int length);
void M_ScreenShot(void);
int M_CacheThumbNail(byte** data);
void M_LoadDefaults(void);
void M_SaveDefaults(void);
boolean M_StringCopy(int8_t* dest, const int8_t* src, size_t dest_size);
int8_t* M_StringDuplicate(const int8_t* orig);

//
// DEFAULTS
//
extern int        DualMouse;

extern int      viewwidth;
extern int      viewheight;

extern int8_t* chat_macros[];

//extern boolean HighSound;

#endif
