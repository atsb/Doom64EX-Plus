// Emacs style mode select   -*- C++ -*- 
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

#ifndef __BLAM_MACRO__
#define __BLAM_MACRO__

#define MAX_MACRO_DATA  255
#define MAX_MACRO_DEFS  255

typedef struct
{
	short id;
	short tag;
	short special;
} macrodata_t;

typedef struct
{
	short actions;
	macrodata_t data[MAX_MACRO_DATA];
} macrodef_t;

extern macrodef_t macrodefs[MAX_MACRO_DEFS];
extern macrodata_t *macroaction;
extern macrodef_t *activemacro;
extern int m_activemacros;
extern int m_totalactions;
extern int m_currentline;

void M_Init(void);
void M_NewMacro(int id);
void M_AppendAction(int type, int tag);
void M_NewLine(void);
int M_GetLine(void);
void M_FinishMacro(void);
void M_WriteMacroLump(void);
void M_DecompileMacro(const char *file);

#endif