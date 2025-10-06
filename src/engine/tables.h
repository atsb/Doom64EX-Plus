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

#ifndef __TABLES__
#define __TABLES__

#include <SDL3/SDL_platform_defines.h>

#include "m_fixed.h"

#ifndef SDL_PLATFORM_MACOS //ATSB: macOS already defines M_PI
#ifdef M_PI
#undef M_PI
#endif

#define M_PI            3.14159265358979323846
#endif

#define FINEANGLES			8192
#define FINEMASK			(FINEANGLES-1)
#define	ANGLETOFINESHIFT	19	/* 0x100000000 to 0x2000 */

// Binary Angle Measument, BAM.
#define ANG45            0x20000000
#define ANG90            0x40000000
#define ANG180            0x80000000
#define ANG270            0xc0000000
#define ANG1            (ANG45/45)
#define ANG5            (ANG90/18)
#define ANGLE_MAX        (0xffffffff)

#define SLOPERANGE        2048
#define SLOPEBITS        11
#define DBITS            (FRACBITS-SLOPEBITS)

#define TRUEANGLES(x) (((x) >> ANGLETOFINESHIFT) * 360.0f / FINEANGLES)

typedef unsigned angle_t;

// Effective size is 10240.
extern  fixed_t        finesine[5 * FINEANGLES / 4];

// Re-use data, is just PI/2 pahse shift.
extern  fixed_t* finecosine;

// Effective size is 2049;
// The +1 size is to handle the case when x==y
//  without additional checking.
extern angle_t        tantoangle[SLOPERANGE + 1];

// Utility function,
//  called by R_PointToAngle.
int
SlopeDiv
(unsigned    num,
	unsigned    den);

#endif
