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

#ifndef __I_SWAP_H__
#define __I_SWAP_H__

#include <SDL3/SDL_endian.h>

#define I_SwapLE16(x)   SDL_Swap16LE(x)
#define I_SwapLE32(x)   SDL_Swap32LE(x)
#define I_SwapBE16(x)   SDL_Swap16BE(x)
#define I_SwapBE32(x)   SDL_Swap32BE(x)

#define SHORT(x)        ((short)I_SwapLE16(x))
#define LONG(x)         ((signed long)I_SwapLE32(x))

// Defines for checking the endianness of the system.

#if SDL_BYTEORDER == SDL_LIL_ENDIAN
#define SYS_LITTLE_ENDIAN
#elif SDL_BYTEORDER == SDL_BIG_ENDIAN
#define SYS_BIG_ENDIAN
#endif

#endif // __I_SWAP_H__
