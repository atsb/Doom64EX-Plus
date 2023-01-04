// Emacs style mode select   -*- C -*- 
//-----------------------------------------------------------------------------
//
// Copyright(C) 2012 Samuel Villarreal
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

#ifndef __BLAM_TYPE__
#define __BLAM_TYPE__

#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#ifdef _WIN32
#include <windows.h>
#include <rpc.h>
#include <rpcndr.h>
#endif
extern int myargc;
extern char **myargv;

#ifdef C89
#define false 0
#define true 1
#ifndef _WIN32
typedef char boolean;
#endif
#else
typedef char boolean;
#endif

#ifdef _WIN32
#ifdef OLD_MSVC
typedef BYTE byte;
typedef WORD word;
typedef DWORD dword;
#else
typedef UINT8 byte;
typedef UINT16 word;
typedef UINT64 dword;
#endif
#else
typedef unsigned char   byte;
typedef unsigned short  word;
typedef unsigned long   dword;
#endif
#ifdef _WIN32

#define DIR_SEPARATOR '\\'
#define PATH_SEPARATOR ';'

#else

#define DIR_SEPARATOR '/'
#define PATH_SEPARATOR ':'

#endif

#ifdef _MSC_VER
    #include <direct.h>
    #include <io.h>
    #define strcasecmp  _stricmp
    #define strncasecmp _strnicmp
#endif


static int Swap16(int x)
{
    return (((word)(x & 0xff) << 8) | ((word)x >> 8));
}

static unsigned int Swap32(unsigned int x)
{
    return(
        ((x & 0xff) << 24)          |
        (((x >> 8) & 0xff) << 16)   |
        (((x >> 16) & 0xff) << 8)   |
        ((x >> 24) & 0xff)
        );
}

#endif
