// Emacs style mode select   -*- C -*-
//-----------------------------------------------------------------------------
//
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

#ifndef __SC_MAIN__
#define __SC_MAIN__

#include <stdint.h>
#include "doomtype.h"

typedef struct {
	const char* token;
	int64_t   ptroffset;
	char    type;
} scdatatable_t;

typedef struct {
	char    token[512];
	byte* buffer;
	unsigned char* pointer_start;
	unsigned char* pointer_end;
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
