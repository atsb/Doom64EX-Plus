// Emacs style mode select   -*- C -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 2022 André Guilherme 
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
#else
#include <fcntl.h>
#include <unistd.h>
#endif

#ifdef OLD_MSVC
#define W32GetVersionEX(lpVersionInformation) GetVersionEx(lpVersionInformation)
#else
#define W32GetVersionEX(lpVersionInformation, wTypeMask, dwlConditionMask) VerifyVersionInfo(lpVersionInformation, wTypeMask, dwlConditionMask)
#define W32OVERSIONINFO LPOSVERSIONINFOEXW
#endif
#ifdef C89 //Adding again because some platforms don´t support stdbool.h
#define false 0
#define true 1
typedef unsigned char boolean;
#else
typedef unsigned char boolean;
#endif

#ifdef _WIN32
#ifdef _XBOX
#include <xtl.h>
#else
#include <Windows.h>
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
#ifndef _XBOX
#define w3sopen(FileName, OpenFlag, ...) _open(FileName, OpenFlag, __VA_ARGS__)
#define w3swrite(handle, buf, maxcharcount) _write(handle, buf, maxcharcount)
#define w3saccess(filename, accessmode) _access(filename, accessmode)
#define w3sread(filehandle, dstbuf, maxcharcount) _read(filehandle, dstbuf, maxcharcount)
#define w3sclose(filehandle) _close(filehandle)
#define w3sstrdup(source) _strdup(source)
#define w3sstricmp(str1, str2, size) _stricmp(str1, str2)
#define w3sstrupr(str) _strupr(str)
#define w3sstrnicmp(str1, str2, size) _strnicmp(str1, str2, size)
#define w3sstrcasecmp(str1, str2, size) w3sstricmp(str1, str2, size)
#define w3sstrncasecmp(str1, str2, size) w3sstrnicmp(str1, str2, size)
#define w3ssnprintf(buf, buffcount, format) _snprintf(buf, buffcount, format)
#define w3svsnprintf(buf, buffcount, format, arglist) _vsnprintf(buf, buffcount, format, arglist)
#define w3sstrlwr(str) _strlwr(str)
#define DIR_SEPARATOR '\\'
#define PATH_SEPARATOR ';'
#else
#define w3sopen(FileName, OpenFlag, __VA_ARGS__) open(FileName, OpenFlag, __VA_ARGS__)
#define w3swrite(handle, buf, maxcharcount) write(handle, buf, maxcharcount)
#define w3saccess(filename, accessmode) access(filename, accessmode)
#define w3sread(filehandle, dstbuf, maxcharcount) read(filehandle, dstbuf, maxcharcount)
#define w3sclose(filehandle) close(filehandle)
#define w3sstrdup(source) strdup(source)
#define DIR_SEPARATOR '\\'
#define PATH_SEPARATOR ';'
#endif
#else
#define w3sopen(FileName, OpenFlag, ...) open(FileName, OpenFlag, __VA_ARGS__)
#define w3swrite(handle, buf, maxcharcount) write(handle, buf, maxcharcount)
#define w3saccess(filename, accessmode) access(filename, accessmode)
#define w3sclose(filehandle) close(filehandle)
#define w3sclose(filehandle) close(filehandle)
#define w3sread(filehandle, dstbuf, maxcharcount) read(filehandle, dstbuf, maxcharcount)
#define w3sstrdup(source) strdup(source)
#define w3sstrupr(str) strupr(str)
#define w3sstricmp(str1, str2) stricmp(str1, str2)
#define w3sstrnicmp(str1, str2, size) stricmp(str1, str2)
#define w3sstrnicmp(str1, str2, size) strnicmp(str1, str2, size)
#define w3ssnprintf(buf, buffcount, format) snprintf(buf, buffcount, format)
#define w3svsnprintf vsnprintf(buf, buffcount, format, arglist)
#define w3sstrlwr(str) strlwr(str)
#define w3sstrcasecmp(str1, str2, size) w3sstricmp(str1, str2, size)
#define w3sstrncasecmp(str1, str2, size) w3sstrnicmp(str1, str2, size)
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
#define Free(userdir)	free(userdir)
#else
#define Free(userdir)	SDL_free(userdir)
#endif

#ifdef _WIN32 //TBD: Sony Playstation 2 port
#define w3ssleep(usecs) Sleep(usecs)
#else
void w3ssleep(dword usecs);
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
