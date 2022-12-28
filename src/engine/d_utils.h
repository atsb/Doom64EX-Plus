// Emacs style mode select   -*- C -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 2007-2012 Samuel Villarreal
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

#ifndef __D_UTILS__H
#define __D_UTILS__h
// build version
#include "doomtype.h"
extern const int8_t version_date[];

void        _dprintf(const int8_t* s, ...);
void* dmemcpy(void* s1, const void* s2, size_t n);
void* dmemset(void* s, dword c, size_t n);
int8_t* dstrcpy(int8_t* dest, const int8_t* src);
void        dstrncpy(int8_t* dest, const int8_t* src, int maxcount);
int         dstrcmp(const int8_t* s1, const int8_t* s2);
int         dstrncmp(const int8_t* s1, const int8_t* s2, int len);
int         dstricmp(const int8_t* s1, const int8_t* s2);
int         dstrnicmp(const int8_t* s1, const int8_t* s2, int len);
void        dstrupr(int8_t* s);
void        dstrlwr(int8_t* s);
int         dstrlen(const int8_t* string);
int8_t* dstrrchr(int8_t* s, int8_t c);
void        dstrcat(int8_t* dest, const int8_t* src);
int8_t* dstrstr(int8_t* s1, int8_t* s2);
int         datoi(const int8_t* str);
float       datof(int8_t* str);
int         dhtoi(int8_t* str);
boolean    dfcmp(float f1, float f2);
int         dsprintf(int8_t* buf, const int8_t* format, ...);
int         dsnprintf(int8_t* src, size_t n, const int8_t* str, ...);

extern int D_abs(int x);
extern float D_fabs(float x);

#define dcos(angle) finecosine[(angle) >> ANGLETOFINESHIFT]
#define dsin(angle) finesine[(angle) >> ANGLETOFINESHIFT]
#endif