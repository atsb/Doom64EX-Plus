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

#ifndef __P_MACROS__
#define __P_MACROS__

#include "p_mobj.h"
#include "d_think.h"
#include "t_bsp.h"

#define MAXQUEUELIST    16

extern int taglist[MAXQUEUELIST];
extern int taglistidx;

void P_QueueSpecial(mobj_t* mobj);

extern thinker_t* macrothinker;
extern macrodef_t* macro;
extern macrodata_t* nextmacro;
extern mobj_t* mobjmacro;
extern short        macrocounter;
extern short        macroid;

void P_InitMacroVars(void);
void P_ToggleMacros(int tag, boolean toggleon);
void P_MacroDetachThinker(thinker_t* thinker);
void P_RunMacros(void);

int P_StartMacro(mobj_t* thing, line_t* line);
int P_SuspendMacro(int tag);
int P_InitMacroCounter(int counts);

#endif
