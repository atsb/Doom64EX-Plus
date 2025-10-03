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

#ifndef __DOOMTYPE__
#define __DOOMTYPE__

typedef unsigned char		boolean;
typedef unsigned char		byte;
typedef unsigned short		word;

#ifndef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))
#endif

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

#ifndef BETWEEN
#define BETWEEN(l,u,x) ((l)>(x)?(l):(x)>(u)?(u):(x))
#endif

#if defined(_MSC_VER)
#define ALIGNED(x) __declspec(align(x))
#define ALIGNOF(x) __alignof(x)
#else

#if defined(__GNUC__) || defined(__clang__)
#include <stdalign.h>
#define ALIGNED(x) __attribute__((aligned(x)))
#if ((__STDC_VERSION__ >= 202000) || defined (__alignof_is_defined))
#define ALIGNOF(x) __alignof__(x)
#else
#error "Unknown compiler can't define ALIGN/ALIGNOF"
#endif
#endif
#endif

#endif
