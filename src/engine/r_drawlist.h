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

#ifndef _R_DRAWLIST_H_
#define _R_DRAWLIST_H_

#include "doomtype.h"
#include "gl_main.h"

typedef enum {
	DLF_GLOW = 0x1,
	DLF_WATER1 = 0x2,
	DLF_CEILING = 0x4,
	DLF_MIRRORS = 0x8,
	DLF_MIRRORT = 0x10,
	DLF_WATER2 = 0x20
} drawlistflag_e;

typedef enum {
	DLT_WALL,
	DLT_FLAT,
	DLT_SPRITE,
	DLT_AMAP,
	NUMDRAWLISTS
} drawlisttag_e;

typedef boolean (*vtxlist_callback_t) (void*, vtx_t*);

typedef struct {
	void* data;
	vtxlist_callback_t callback;
	dtexture    texid;
	int         flags;
	int         params;
} vtxlist_t;

typedef struct {
	vtxlist_t* list;
	int         index;
	int         max;
} drawlist_t;

extern drawlist_t drawlist[NUMDRAWLISTS];

#define MAXDLDRAWCOUNT  0x10000
extern vtx_t drawVertex[MAXDLDRAWCOUNT];

boolean DL_ProcessWalls(vtxlist_t* vl, int* drawcount);
boolean DL_ProcessLeafs(vtxlist_t* vl, int* drawcount);
boolean DL_ProcessSprites(vtxlist_t* vl, int* drawcount);

vtxlist_t* DL_AddVertexList(drawlist_t* dl);
int DL_GetDrawListSize(int tag);
void DL_BeginDrawList(boolean t, boolean a);
void DL_ProcessDrawList(int tag, boolean(*procfunc)(vtxlist_t*, int*));
void DL_RenderDrawList(void);
void DL_Init(void);

#endif
