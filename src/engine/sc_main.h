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
	const char* token;
	int64_t   ptroffset;
	char    type;
} scdatatable_t;

typedef struct {
	char    token[512];
	byte* buffer;
	char* pointer_start;
	char* pointer_end;
	int     linepos;
	int     rowpos;
	int     buffpos;
	int     buffsize;
	void (*open)(const char*);
	void (*close)(void);
	void (*compare)(const char*);
	int (*find)(boolean);
	char(*fgetchar)(void);
	void (*rewind)(void);
	char* (*getstring)(void);
	int (*getint)(void);
	int (*setdata)(void*, const scdatatable_t*);
	int (*readtokens)(void);
	void (*error)(const char*);
} scparser_t;

extern scparser_t sc_parser;

void SC_Init(void);

#endif // __SC_MAIN__
