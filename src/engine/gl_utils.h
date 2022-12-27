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

#ifndef VITA
//
// GL_ARB_multitexture
//
extern PFNGLACTIVETEXTUREARBPROC _glActiveTextureARB;
extern PFNGLCLIENTACTIVETEXTUREARBPROC _glClientActiveTextureARB;
extern PFNGLMULTITEXCOORD1DARBPROC _glMultiTexCoord1dARB;
extern PFNGLMULTITEXCOORD1DVARBPROC _glMultiTexCoord1dvARB;
extern PFNGLMULTITEXCOORD1FARBPROC _glMultiTexCoord1fARB;
extern PFNGLMULTITEXCOORD1FVARBPROC _glMultiTexCoord1fvARB;
extern PFNGLMULTITEXCOORD1IARBPROC _glMultiTexCoord1iARB;
extern PFNGLMULTITEXCOORD1IVARBPROC _glMultiTexCoord1ivARB;
extern PFNGLMULTITEXCOORD1SARBPROC _glMultiTexCoord1sARB;
extern PFNGLMULTITEXCOORD1SVARBPROC _glMultiTexCoord1svARB;
extern PFNGLMULTITEXCOORD2DARBPROC _glMultiTexCoord2dARB;
extern PFNGLMULTITEXCOORD2DVARBPROC _glMultiTexCoord2dvARB;
extern PFNGLMULTITEXCOORD2FARBPROC _glMultiTexCoord2fARB;
extern PFNGLMULTITEXCOORD2FVARBPROC _glMultiTexCoord2fvARB;
extern PFNGLMULTITEXCOORD2IARBPROC _glMultiTexCoord2iARB;
extern PFNGLMULTITEXCOORD2IVARBPROC _glMultiTexCoord2ivARB;
extern PFNGLMULTITEXCOORD2SARBPROC _glMultiTexCoord2sARB;
extern PFNGLMULTITEXCOORD2SVARBPROC _glMultiTexCoord2svARB;
extern PFNGLMULTITEXCOORD3DARBPROC _glMultiTexCoord3dARB;
extern PFNGLMULTITEXCOORD3DVARBPROC _glMultiTexCoord3dvARB;
extern PFNGLMULTITEXCOORD3FARBPROC _glMultiTexCoord3fARB;
extern PFNGLMULTITEXCOORD3FVARBPROC _glMultiTexCoord3fvARB;
extern PFNGLMULTITEXCOORD3IARBPROC _glMultiTexCoord3iARB;
extern PFNGLMULTITEXCOORD3IVARBPROC _glMultiTexCoord3ivARB;
extern PFNGLMULTITEXCOORD3SARBPROC _glMultiTexCoord3sARB;
extern PFNGLMULTITEXCOORD3SVARBPROC _glMultiTexCoord3svARB;
extern PFNGLMULTITEXCOORD4DARBPROC _glMultiTexCoord4dARB;
extern PFNGLMULTITEXCOORD4DVARBPROC _glMultiTexCoord4dvARB;
extern PFNGLMULTITEXCOORD4FARBPROC _glMultiTexCoord4fARB;
extern PFNGLMULTITEXCOORD4FVARBPROC _glMultiTexCoord4fvARB;
extern PFNGLMULTITEXCOORD4IARBPROC _glMultiTexCoord4iARB;
extern PFNGLMULTITEXCOORD4IVARBPROC _glMultiTexCoord4ivARB;
extern PFNGLMULTITEXCOORD4SARBPROC _glMultiTexCoord4sARB;
extern PFNGLMULTITEXCOORD4SVARBPROC _glMultiTexCoord4svARB;

#define GL_ARB_multitexture_Define() \
PFNGLACTIVETEXTUREARBPROC _glActiveTextureARB = NULL; \
PFNGLCLIENTACTIVETEXTUREARBPROC _glClientActiveTextureARB = NULL; \
PFNGLMULTITEXCOORD1DARBPROC _glMultiTexCoord1dARB = NULL; \
PFNGLMULTITEXCOORD1DVARBPROC _glMultiTexCoord1dvARB = NULL; \
PFNGLMULTITEXCOORD1FARBPROC _glMultiTexCoord1fARB = NULL; \
PFNGLMULTITEXCOORD1FVARBPROC _glMultiTexCoord1fvARB = NULL; \
PFNGLMULTITEXCOORD1IARBPROC _glMultiTexCoord1iARB = NULL; \
PFNGLMULTITEXCOORD1IVARBPROC _glMultiTexCoord1ivARB = NULL; \
PFNGLMULTITEXCOORD1SARBPROC _glMultiTexCoord1sARB = NULL; \
PFNGLMULTITEXCOORD1SVARBPROC _glMultiTexCoord1svARB = NULL; \
PFNGLMULTITEXCOORD2DARBPROC _glMultiTexCoord2dARB = NULL; \
PFNGLMULTITEXCOORD2DVARBPROC _glMultiTexCoord2dvARB = NULL; \
PFNGLMULTITEXCOORD2FARBPROC _glMultiTexCoord2fARB = NULL; \
PFNGLMULTITEXCOORD2FVARBPROC _glMultiTexCoord2fvARB = NULL; \
PFNGLMULTITEXCOORD2IARBPROC _glMultiTexCoord2iARB = NULL; \
PFNGLMULTITEXCOORD2IVARBPROC _glMultiTexCoord2ivARB = NULL; \
PFNGLMULTITEXCOORD2SARBPROC _glMultiTexCoord2sARB = NULL; \
PFNGLMULTITEXCOORD2SVARBPROC _glMultiTexCoord2svARB = NULL; \
PFNGLMULTITEXCOORD3DARBPROC _glMultiTexCoord3dARB = NULL; \
PFNGLMULTITEXCOORD3DVARBPROC _glMultiTexCoord3dvARB = NULL; \
PFNGLMULTITEXCOORD3FARBPROC _glMultiTexCoord3fARB = NULL; \
PFNGLMULTITEXCOORD3FVARBPROC _glMultiTexCoord3fvARB = NULL; \
PFNGLMULTITEXCOORD3IARBPROC _glMultiTexCoord3iARB = NULL; \
PFNGLMULTITEXCOORD3IVARBPROC _glMultiTexCoord3ivARB = NULL; \
PFNGLMULTITEXCOORD3SARBPROC _glMultiTexCoord3sARB = NULL; \
PFNGLMULTITEXCOORD3SVARBPROC _glMultiTexCoord3svARB = NULL; \
PFNGLMULTITEXCOORD4DARBPROC _glMultiTexCoord4dARB = NULL; \
PFNGLMULTITEXCOORD4DVARBPROC _glMultiTexCoord4dvARB = NULL; \
PFNGLMULTITEXCOORD4FARBPROC _glMultiTexCoord4fARB = NULL; \
PFNGLMULTITEXCOORD4FVARBPROC _glMultiTexCoord4fvARB = NULL; \
PFNGLMULTITEXCOORD4IARBPROC _glMultiTexCoord4iARB = NULL; \
PFNGLMULTITEXCOORD4IVARBPROC _glMultiTexCoord4ivARB = NULL; \
PFNGLMULTITEXCOORD4SARBPROC _glMultiTexCoord4sARB = NULL; \
PFNGLMULTITEXCOORD4SVARBPROC _glMultiTexCoord4svARB = NULL

#define GL_ARB_multitexture_Init() \
GL_CheckExtension("GL_ARB_multitexture"); \
_glActiveTextureARB = GL_RegisterProc("glActiveTextureARB"); \
_glClientActiveTextureARB = GL_RegisterProc("glClientActiveTextureARB"); \
_glMultiTexCoord1dARB = GL_RegisterProc("glMultiTexCoord1dARB"); \
_glMultiTexCoord1dvARB = GL_RegisterProc("glMultiTexCoord1dvARB"); \
_glMultiTexCoord1fARB = GL_RegisterProc("glMultiTexCoord1fARB"); \
_glMultiTexCoord1fvARB = GL_RegisterProc("glMultiTexCoord1fvARB"); \
_glMultiTexCoord1iARB = GL_RegisterProc("glMultiTexCoord1iARB"); \
_glMultiTexCoord1ivARB = GL_RegisterProc("glMultiTexCoord1ivARB"); \
_glMultiTexCoord1sARB = GL_RegisterProc("glMultiTexCoord1sARB"); \
_glMultiTexCoord1svARB = GL_RegisterProc("glMultiTexCoord1svARB"); \
_glMultiTexCoord2dARB = GL_RegisterProc("glMultiTexCoord2dARB"); \
_glMultiTexCoord2dvARB = GL_RegisterProc("glMultiTexCoord2dvARB"); \
_glMultiTexCoord2fARB = GL_RegisterProc("glMultiTexCoord2fARB"); \
_glMultiTexCoord2fvARB = GL_RegisterProc("glMultiTexCoord2fvARB"); \
_glMultiTexCoord2iARB = GL_RegisterProc("glMultiTexCoord2iARB"); \
_glMultiTexCoord2ivARB = GL_RegisterProc("glMultiTexCoord2ivARB"); \
_glMultiTexCoord2sARB = GL_RegisterProc("glMultiTexCoord2sARB"); \
_glMultiTexCoord2svARB = GL_RegisterProc("glMultiTexCoord2svARB"); \
_glMultiTexCoord3dARB = GL_RegisterProc("glMultiTexCoord3dARB"); \
_glMultiTexCoord3dvARB = GL_RegisterProc("glMultiTexCoord3dvARB"); \
_glMultiTexCoord3fARB = GL_RegisterProc("glMultiTexCoord3fARB"); \
_glMultiTexCoord3fvARB = GL_RegisterProc("glMultiTexCoord3fvARB"); \
_glMultiTexCoord3iARB = GL_RegisterProc("glMultiTexCoord3iARB"); \
_glMultiTexCoord3ivARB = GL_RegisterProc("glMultiTexCoord3ivARB"); \
_glMultiTexCoord3sARB = GL_RegisterProc("glMultiTexCoord3sARB"); \
_glMultiTexCoord3svARB = GL_RegisterProc("glMultiTexCoord3svARB"); \
_glMultiTexCoord4dARB = GL_RegisterProc("glMultiTexCoord4dARB"); \
_glMultiTexCoord4dvARB = GL_RegisterProc("glMultiTexCoord4dvARB"); \
_glMultiTexCoord4fARB = GL_RegisterProc("glMultiTexCoord4fARB"); \
_glMultiTexCoord4fvARB = GL_RegisterProc("glMultiTexCoord4fvARB"); \
_glMultiTexCoord4iARB = GL_RegisterProc("glMultiTexCoord4iARB"); \
_glMultiTexCoord4ivARB = GL_RegisterProc("glMultiTexCoord4ivARB"); \
_glMultiTexCoord4sARB = GL_RegisterProc("glMultiTexCoord4sARB"); \
_glMultiTexCoord4svARB = GL_RegisterProc("glMultiTexCoord4svARB")

#define glActiveTextureARB(texture) _glActiveTextureARB(texture)
#define glClientActiveTextureARB(texture) _glClientActiveTextureARB(texture)
#define glMultiTexCoord1dARB(target, s) _glMultiTexCoord1dARB(target, s)
#define glMultiTexCoord1dvARB(target, v) _glMultiTexCoord1dvARB(target, v)
#define glMultiTexCoord1fARB(target, s) _glMultiTexCoord1fARB(target, s)
#define glMultiTexCoord1fvARB(target, v) _glMultiTexCoord1fvARB(target, v)
#define glMultiTexCoord1iARB(target, s) _glMultiTexCoord1iARB(target, s)
#define glMultiTexCoord1ivARB(target, v) _glMultiTexCoord1ivARB(target, v)
#define glMultiTexCoord1sARB(target, s) _glMultiTexCoord1sARB(target, s)
#define glMultiTexCoord1svARB(target, v) _glMultiTexCoord1svARB(target, v)
#define glMultiTexCoord2dARB(target, s, t) _glMultiTexCoord2dARB(target, s, t)
#define glMultiTexCoord2dvARB(target, v) _glMultiTexCoord2dvARB(target, v)
#define glMultiTexCoord2fARB(target, s, t) _glMultiTexCoord2fARB(target, s, t)
#define glMultiTexCoord2fvARB(target, v) _glMultiTexCoord2fvARB(target, v)
#define glMultiTexCoord2iARB(target, s, t) _glMultiTexCoord2iARB(target, s, t)
#define glMultiTexCoord2ivARB(target, v) _glMultiTexCoord2ivARB(target, v)
#define glMultiTexCoord2sARB(target, s, t) _glMultiTexCoord2sARB(target, s, t)
#define glMultiTexCoord2svARB(target, v) _glMultiTexCoord2svARB(target, v)
#define glMultiTexCoord3dARB(target, s, t, r) _glMultiTexCoord3dARB(target, s, t, r)
#define glMultiTexCoord3dvARB(target, v) _glMultiTexCoord3dvARB(target, v)
#define glMultiTexCoord3fARB(target, s, t, r) _glMultiTexCoord3fARB(target, s, t, r)
#define glMultiTexCoord3fvARB(target, v) _glMultiTexCoord3fvARB(target, v)
#define glMultiTexCoord3iARB(target, s, t, r) _glMultiTexCoord3iARB(target, s, t, r)
#define glMultiTexCoord3ivARB(target, v) _glMultiTexCoord3ivARB(target, v)
#define glMultiTexCoord3sARB(target, s, t, r) _glMultiTexCoord3sARB(target, s, t, r)
#define glMultiTexCoord3svARB(target, v) _glMultiTexCoord3svARB(target, v)
#define glMultiTexCoord4dARB(target, s, t, r, q) _glMultiTexCoord4dARB(target, s, t, r, q)
#define glMultiTexCoord4dvARB(target, v) _glMultiTexCoord4dvARB(target, v)
#define glMultiTexCoord4fARB(target, s, t, r, q) _glMultiTexCoord4fARB(target, s, t, r, q)
#define glMultiTexCoord4fvARB(target, v) _glMultiTexCoord4fvARB(target, v)
#define glMultiTexCoord4iARB(target, s, t, r, q) _glMultiTexCoord4iARB(target, s, t, r, q)
#define glMultiTexCoord4ivARB(target, v) _glMultiTexCoord4ivARB(target, v)
#define glMultiTexCoord4sARB(target, s, t, r, q) _glMultiTexCoord4sARB(target, s, t, r, q)
#define glMultiTexCoord4svARB(target, v) _glMultiTexCoord4svARB(target, v)
#endif
#endif