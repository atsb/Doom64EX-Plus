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

#ifndef __I_PNG_H__
#define __I_PNG_H__

#include <stdbool.h>
#include <png.h>
#include "doomtype.h"

byte* I_PNGReadData(int lump, bool palette, bool nopack, bool alpha,
	int* w, int* h, int* offset, int palindex);

byte* I_PNGCreate(int width, int height, byte* data, int* size);

#endif // __I_PNG_H__
