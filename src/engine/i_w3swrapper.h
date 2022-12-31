// Emacs style mode select   -*- C -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 2022-2023 André Guilherme 
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

#ifdef USE_STDINT 
#include <stdint.h>
#else
#include "doomtype.h"
#endif
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#include <string.h>
#else
#include <fcntl.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#endif
#include <stdarg.h>

#ifdef OLD_MSVC
#define W32GetVersionEX(lpVersionInformation) GetVersionEx(lpVersionInformation)
#else
#define W32GetVersionEX(lpVersionInformation, wTypeMask, dwlConditionMask) VerifyVersionInfo(lpVersionInformation, wTypeMask, dwlConditionMask)
#define W32OVERSIONINFO LPOSVERSIONINFOEXW
#endif
#ifdef C89 //Adding again because some platforms don´t support stdbool.h
#ifndef _WIN32
#define false 0
#define true 1
typedef unsigned char boolean;
#endif
#else
#ifndef _WIN32
typedef unsigned char boolean;
#endif
#endif

#ifdef _WIN32
#ifdef _XBOX
#include <xtl.h>
#include <rpc.h>
#include <rpcndr.h>
#else
#include <Windows.h>
#include <rpc.h>
#include <rpcndr.h>
#endif

#ifdef OLD_TYPE
typedef BYTE w3suint8_t;
typedef WORD w3suint16_t;
typedef DWORD w3suint64_t;
#else
typedef UINT8  w3suint8_t;
typedef UINT16 w3suint16_t;
typedef UINT64 w3suint64_t;
#endif
#elif defined(USE_STDINT)
typedef uint8_t  w3suint8_t;
typedef uint16_t w3suint16_t;
typedef uint64_t w3suint64_t;
#else
typedef unsigned char w3suint8_t;
typedef unsigned short w3suint16_t;
typedef unsigned long long w3suint64_t;
#endif 

typedef w3suint8_t  byte;
typedef w3suint16_t word;
typedef w3suint64_t dword;

#ifdef _WIN32
#define w3sopen _open
#define w3swrite _write
#define w3saccess _access
#define w3sread _read
#define w3sclose _close
#define w3sstrdup _strdup
#define w3sstrupr _strupr
#define w3sstricmp _stricmp
#define w3sstrnicmp _strnicmp
#define w3sstrcasecmp _stricmp
#define w3sstrncasecmp _strnicmp
#define w3ssnprintf _snprintf
#define w3svsnprintf _vsnprintf
#define w3sstrlwr _strlwr
#define DIR_SEPARATOR '\\'
#define PATH_SEPARATOR ';'
#else
#define w3sopen open
#define w3swrite write
#define w3saccess access
#define w3sclose) close
#define w3sread read
#define w3sstrdup strdup
char* w3sstrupr(char *str);
#define w3ssnprintf snprintf
#define w3svsnprintf vsnprintf
char* w3sstrlwr(char *str);
#define w3sstricmp stricmp
#define w3sstrnicmp strnicmp
#define w3sstrcasecmp strcasecmp
#define w3sstrncasecmp strncasecmp
#define DIR_SEPARATOR '/'
#define PATH_SEPARATOR ':'
#endif

#ifdef DOOM_UNIX_INSTALL
#define GetBasePath()	SDL_GetPrefPath("", "doom64ex-plus")
#elif !defined DOOM_UNIX_INSTALL || defined _WIN32 || !defined __ANDROID__
#define GetBasePath()	SDL_GetBasePath()
#elif defined __ANDROID__
#define GetBasePath()   SDL_AndroidGetInternalStoragePath()
#endif

#if defined(__linux__) || defined(__OpenBSD__)
#define Free	free
#else
#define Free	SDL_free
#endif

#ifdef _WIN32 //TBD: Sony Playstation 2 port
#define w3ssleep Sleep
#else
void w3ssleep(dword usecs);
#endif

#if defined(__GNUC__) //By James Haley
#define d_inline __inline__
#elif defined(_WIN32)
#ifdef OLD_MSVC
#define d_inline __inline
#else
#define d_inline inline
#endif
#else
#define d_inline
#endif


#ifdef NO_VSNPRINTF
#define w3svsnprintf(buf, format, arg) vsprintf(buf, format, arg)
#else
int M_vsnprintf(char* buf, size_t buf_len, const char* s, va_list args);
#endif

int         htoi(int8_t* str);
boolean    fcmp(float f1, float f2);


#define dcos(angle) finecosine[(angle) >> ANGLETOFINESHIFT]
#define dsin(angle) finesine[(angle) >> ANGLETOFINESHIFT]
