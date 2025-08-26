// Emacs style mode select   -*- C -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 1993-1997 Id Software, Inc.
// Copyright(C) 1997 Midway Home Entertainment, Inc
// Copyright(C) 2007-2012 Samuel Villarreal
//
// This program is free software; you can redistribute it and/or
// // Copyright (C) 1993-1996 by id Software, Inc.
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


#ifndef __D_MAIN__
#define __D_MAIN__

#include "d_event.h"

//
// D_DoomMain()
// Not a globally visible function, just included for source reference,
// calls all startup code, parses command line options.
// If not overrided by user input, calls N_AdvanceDemo.
//
void D_DoomMain(void);

int D_MiniLoop(void (*start)(void), void (*stop)(void),
               void (*draw)(void), int(*tick)(void));

// Called by IO functions when input is detected.
void D_PostEvent(event_t* ev);


//
// BASE LEVEL
//

void D_IncValidCount(void);

extern boolean BusyDisk;

// atsb: SHADERS
void D_ShaderBind(void);

typedef char GLchar;
typedef unsigned int GLuint;
typedef int GLint;
typedef unsigned int GLenum;
typedef int GLsizei;

static struct {
    GLuint prog;
    GLint  locTex;
    int    initialised;
} shader_struct = { 0, -1, 0 };

#ifndef GL_VERTEX_SHADER
#define GL_VERTEX_SHADER    0x8B31
#define GL_FRAGMENT_SHADER  0x8B30
#define GL_COMPILE_STATUS   0x8B81
#define GL_LINK_STATUS      0x8B82
#define GL_INFO_LOG_LENGTH  0x8B84
#endif

#endif
