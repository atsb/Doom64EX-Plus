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

#ifndef __SC_MAIN__
#define __SC_MAIN__

typedef struct {
	int8_t    token[512];
	byte* buffer;
	int8_t* pointer_start;
	int8_t* pointer_end;
	int     linepos;
	int     rowpos;
	int     buffpos;
	int     buffsize;
	void (*open)(const int8_t*);
	void (*close)(void);
	void (*compare)(const int8_t*);
	int (*find)(boolean);
	int8_t(*fgetchar)(void);
	void (*rewind)(void);
	int8_t* (*getstring)(void);
	int (*getint)(void);
	int (*setdata)(void*, void*);
	int (*readtokens)(void);
	void (*error)(const int8_t*);
} scparser_t;

extern scparser_t sc_parser;

typedef struct {
	const int8_t* token;
	int64_t   ptroffset;
	int8_t    type;
} scdatatable_t;

void SC_Init(void);

#endif // __SC_MAIN__
