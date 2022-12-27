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


#if defined(__gl_h)
#error "Ogl must be included before of i_w3swrapper.h"
#endif

#ifdef USE_STDINT 
#include <stdint.h>
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

#ifdef C89
#define false 0
#define true 1
typedef int dboolean;
typedef int bool;
#else
#include <stdbool.h>
typedef bool dboolean;
#endif

#ifdef _WIN32
#ifdef _XBOX
#include <xtl.h>
#else
#include <Windows.h>
#endif

#ifdef OLD_TYPE
typedef BYTE byte;
typedef WORD word;
typedef DWORD dword;
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

#ifdef _WIN32
#ifndef _XBOX
#define w3sopen(FileName, OpenFlag, ...) _open(FileName, OpenFlag, __VA_ARGS__)
#define w3swrite(handle, buf, maxcharcount) _write(handle, buf, maxcharcount)
#define w3saccess(filename, accessmode) _access(filename, accessmode)
#define w3sread(filehandle, dstbuf, maxcharcount) _read(filehandle, dstbuf, maxcharcount)
#define w3sclose(filehandle) _close(filehandle)
#define w3sstrdup(source) _strdup(source)
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

#if defined(__LINUX__) || defined(__OpenBSD__)
#define Free(userdir)	free(userdir)
#else
#define Free(userdir)	SDL_free(userdir)
#endif

#ifdef _WIN32 //TBD: Sony Playstation 2 port
#define w3ssleep(usecs) Sleep(usecs)
#else
void w3ssleep(dword usecs);
#endif
#ifdef HAVE_VSNPRINTF
#ifdef WIN32
#define w3svsnprintf(buffer, buffercount, format, arglist)  _vsnprintf(buffer, buffercount, format, arglist)
#else
#define w3svsnprintf(buffer, buffercount, format, arglist) vsnprintf(buffer, buffercount, format, arglist);
#endif
#else
#define w3svsnprintf(buf, format, arg) vsprintf(buf, format, arg)
#endif
