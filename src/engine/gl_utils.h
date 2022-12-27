// Emacs style mode select   -*- C -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 2007-2012 Samuel Villarreal
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

#ifndef __GL_UTILS__H
#define __GL_UTILS__H

#include <math.h>

#include "gl_main.h"


//
// CUSTOM ROUTINES
//
void glSetVertex(vtx_t* vtx);
void glTriangle(int v0, int v1, int v2);
void glDrawGeometry(dword count, vtx_t* vtx);
void glViewFrustum(int width, int height, rfloat fovy, rfloat znear);
void glSetVertexColor(vtx_t* v, rcolor c, word count);
void glGetColorf(rcolor color, float* argb);
void glTexCombReplace(void);
void glTexCombColor(int t, rcolor c, int func);
void glTexCombColorf(int t, float* f, int func);
void glTexCombModulate(int t, int s);
void glTexCombAdd(int t, int s);
void glTexCombInterpolate(int t, float a);
void glTexCombReplaceAlpha(int t);


//
// GL_ARB_multitexture
//
extern PFNGLACTIVETEXTUREARBPROC _glActiveTextureARB;

#define glActiveTextureARB(texture) _glActiveTextureARB(texture)

#endif