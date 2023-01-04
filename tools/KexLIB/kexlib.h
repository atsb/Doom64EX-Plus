// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// Copyright(C) 2007-2014 Samuel Villarreal
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

#ifndef __KEXLIB_H__
#define __KEXLIB_H__

// narrow down the windows preprocessor bullshit down to just one macro define
#if defined(__WIN32__) || defined(__WIN32) || defined(_WIN32_) || defined(_WIN32) || defined(WIN32)
#define KEX_WIN32
#else
#if defined(__APPLE__)
// lets us know what version of Mac OS X we're compiling on
#include "AvailabilityMacros.h"
#include "TargetConditionals.h"
#if TARGET_OS_IPHONE
// if compiling for iPhone
#define KEX_IPHONE
#else
// if not compiling for iPhone
#define KEX_MACOSX
#if MAC_OS_X_VERSION_MIN_REQUIRED < 1050
# error KexLIB for Mac OS X only supports deploying on 10.5 and above.
#endif // MAC_OS_X_VERSION_MIN_REQUIRED < 1050
#if MAC_OS_X_VERSION_MAX_ALLOWED < 1060
# error KexLIB for Mac OS X must be built with a 10.6 SDK or above.
#endif // MAC_OS_X_VERSION_MAX_ALLOWED < 1060
#endif // TARGET_OS_IPHONE
#endif // defined(__APPLE__)
#endif // WIN32

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#ifdef _MSC_VER
#include "framework/win32/opndir.h"
#else
#include <dirent.h>
#endif
#include <time.h>
#ifndef KEX_WIN32
#include <unistd.h>
#endif
#include <new>

#define MAX_FILEPATH    256
#define MAX_HASH        2048

typedef unsigned char   byte;
typedef unsigned short  word;
typedef unsigned long   ulong;
typedef unsigned int    uint;
typedef unsigned int    dtexture;
typedef unsigned int    rcolor;
typedef char            filepath_t[MAX_FILEPATH];

#if defined(_MSC_VER)
typedef signed __int8 int8_t;
typedef unsigned __int8 uint8_t;
typedef signed __int16 int16_t;
typedef unsigned __int16 uint16_t;
typedef signed __int32 int32_t;
typedef unsigned __int32 uint32_t;
typedef signed __int64 int64_t;
typedef unsigned __int64 uint64_t;
#endif

typedef union {
    int     i;
    float   f;
} fint_t;

#define ASCII_SLASH		47
#define ASCII_BACKSLASH 92

#ifdef KEX_WIN32
#define DIR_SEPARATOR '\\'
#define PATH_SEPARATOR ';'
#else
#define DIR_SEPARATOR '/'
#define PATH_SEPARATOR ':'
#endif

#include <limits.h>
#define D_MININT INT_MIN
#define D_MAXINT INT_MAX

#ifndef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))
#endif

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

#ifndef BETWEEN
#define BETWEEN(l,u,x) ((l)>(x)?(l):(x)>(u)?(u):(x))
#endif

#ifndef BIT
#define BIT(num) (1<<(num))
#endif

#if defined(KEX_WIN32) && !defined(__GNUC__)
#define KDECL __cdecl
#else
#define KDECL
#endif

#ifdef ALIGNED
#undef ALIGNED
#endif

#if defined(_MSC_VER)
#define ALIGNED(x) __declspec(align(x))
#define PACKED
#elif defined(__GNUC__)
#define ALIGNED(x) __attribute__ ((aligned(x)))
#define PACKED __attribute__((packed))
#else
#define ALIGNED(x)
#define PACKED
#endif

// function inlining is available on most platforms, however,
// the GNU C __inline__ is too common and conflicts with a 
// definition in other dependencies, so it needs to be factored
// out into a custom macro definition

#if defined(__GNUC__)
#define d_inline __inline__
#elif defined(_MSC_VER) || defined(KEX_WIN32)
#define d_inline __forceinline
#else
#define d_inline
#endif

#define KEXLIB_NAMESPACE_START(x)  namespace x {
#define KEXLIB_NAMESPACE_END    }

#define RGBA(r,g,b,a) ((rcolor)((((a)&0xff)<<24)|(((b)&0xff)<<16)|(((g)&0xff)<<8)|((r)&0xff)))

#define COLOR_WHITE         RGBA(0xFF, 0xFF, 0xFF, 0xFF)
#define COLOR_WHITE_A(a)    RGBA(0xFF, 0xFF, 0xFF, a)
#define COLOR_RED           RGBA(0xFF, 0, 0, 0xFF)
#define COLOR_GREEN         RGBA(0, 0xFF, 0, 0xFF)
#define COLOR_YELLOW        RGBA(0xFF, 0xFF, 0, 0xFF)
#define COLOR_CYAN          RGBA(0, 0xFF, 0xFF, 0xFF)

#include "math/mathlib.h"
#include "framework/system.h"
#include "framework/kstring.h"
#include "framework/inputSystem.h"
#include "framework/cmd.h"
#include "framework/cvar.h"
#include "framework/array.h"
#include "framework/linkedlist.h"
#include "framework/object.h"
#include "framework/memHeap.h"
#include "framework/keymap.h"
#include "framework/fileSystem.h"
#include "framework/binFile.h"
#include "framework/cachefilelist.h"
#include "framework/keyinput.h"
#include "framework/parser.h"
#include "framework/defs.h"
#include "framework/getter.h"
#include "framework/resourceManager.h"

KEXLIB_NAMESPACE_START(kexlib)

void Init(void);

extern kexCvar cvarDeveloper;
extern kexCvar cvarBasePath;

extern kexSystem *system;
extern kexInputSystem *inputSystem;
extern kexFileSystem *fileSystem;
extern kexCvarManager *cvars;
extern kexCommand *commands;
extern kexInputKey *inputBinds;
extern kexParser *parser;
extern kexDefManager *defs;

KEXLIB_NAMESPACE_END

#endif
