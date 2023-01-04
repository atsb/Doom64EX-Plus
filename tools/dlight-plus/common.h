//
// Copyright (c) 2013-2014 Samuel Villarreal
// svkaiser@gmail.com
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
//    1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
//
//   2. Altered source versions must be plainly marked as such, and must not be
//   misrepresented as being the original software.
//
//    3. This notice may not be removed or altered from any source
//    distribution.
//

#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

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

#ifdef KEX_WIN32
#include <windows.h>
#else
#include <time.h>
#endif

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

typedef union
{
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

#if defined(__GNUC__) || defined(__APPLE__)
#define d_inline __inline__
#elif defined(_MSC_VER) || defined(KEX_WIN32)
#define d_inline __forceinline
#else
#define d_inline
#endif

#include "kexlib/memHeap.h"
#include "kexlib/kstring.h"
#include "kexlib/math/mathlib.h"

void Error(const char *error, ...);
char *Va(const char *str, ...);
void Delay(int ms);
const int64_t GetSeconds(void);
const kexStr &FilePath(void);

#endif
