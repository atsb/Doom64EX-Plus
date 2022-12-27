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
//------------------------------------------------------------------------------

#include <math.h>

#include <SDL_opengl.h>

#include "gl_main.h"
#include "i_system.h"

//
// GL_EXT_compiled_vertex_array
//
extern dboolean has_GL_EXT_compiled_vertex_array;

extern PFNGLLOCKARRAYSEXTPROC _glLockArraysEXT;
extern PFNGLUNLOCKARRAYSEXTPROC _glUnlockArraysEXT;

#define GL_EXT_compiled_vertex_array_Define() \
dboolean has_GL_EXT_compiled_vertex_array = false; \
PFNGLLOCKARRAYSEXTPROC _glLockArraysEXT = NULL; \
PFNGLUNLOCKARRAYSEXTPROC _glUnlockArraysEXT = NULL

#define GL_EXT_compiled_vertex_array_Init() \
has_GL_EXT_compiled_vertex_array = GL_CheckExtension("GL_EXT_compiled_vertex_array"); \
_glLockArraysEXT = GL_RegisterProc("glLockArraysEXT"); \
_glUnlockArraysEXT = GL_RegisterProc("glUnlockArraysEXT")

//
// GL_EXT_multi_draw_arrays
//
extern dboolean has_GL_EXT_multi_draw_arrays;

extern PFNGLMULTIDRAWARRAYSEXTPROC _glMultiDrawArraysEXT;
extern PFNGLMULTIDRAWELEMENTSEXTPROC _glMultiDrawElementsEXT;

#define GL_EXT_multi_draw_arrays_Define() \
dboolean has_GL_EXT_multi_draw_arrays = false; \
PFNGLMULTIDRAWARRAYSEXTPROC _glMultiDrawArraysEXT = NULL; \
PFNGLMULTIDRAWELEMENTSEXTPROC _glMultiDrawElementsEXT = NULL

#define GL_EXT_multi_draw_arrays_Init() \
has_GL_EXT_multi_draw_arrays = GL_CheckExtension("GL_EXT_multi_draw_arrays"); \
_glMultiDrawArraysEXT = GL_RegisterProc("glMultiDrawArraysEXT"); \
_glMultiDrawElementsEXT = GL_RegisterProc("glMultiDrawElementsEXT")

//
// GL_EXT_fog_coord
//
extern dboolean has_GL_EXT_fog_coord;

extern PFNGLFOGCOORDFEXTPROC _glFogCoordfEXT;
extern PFNGLFOGCOORDFVEXTPROC _glFogCoordfvEXT;
extern PFNGLFOGCOORDDEXTPROC _glFogCoorddEXT;
extern PFNGLFOGCOORDDVEXTPROC _glFogCoorddvEXT;
extern PFNGLFOGCOORDPOINTEREXTPROC _glFogCoordPointerEXT;

#define GL_EXT_fog_coord_Define() \
dboolean has_GL_EXT_fog_coord = false; \
PFNGLFOGCOORDFEXTPROC _glFogCoordfEXT = NULL; \
PFNGLFOGCOORDFVEXTPROC _glFogCoordfvEXT = NULL; \
PFNGLFOGCOORDDEXTPROC _glFogCoorddEXT = NULL; \
PFNGLFOGCOORDDVEXTPROC _glFogCoorddvEXT = NULL; \
PFNGLFOGCOORDPOINTEREXTPROC _glFogCoordPointerEXT = NULL

#define GL_EXT_fog_coord_Init() \
has_GL_EXT_fog_coord = GL_CheckExtension("GL_EXT_fog_coord"); \
_glFogCoordfEXT = GL_RegisterProc("glFogCoordfEXT"); \
_glFogCoordfvEXT = GL_RegisterProc("glFogCoordfvEXT"); \
_glFogCoorddEXT = GL_RegisterProc("glFogCoorddEXT"); \
_glFogCoorddvEXT = GL_RegisterProc("glFogCoorddvEXT"); \
_glFogCoordPointerEXT = GL_RegisterProc("glFogCoordPointerEXT")

//
// GL_ARB_vertex_buffer_object
//
extern dboolean has_GL_ARB_vertex_buffer_object;

extern PFNGLBINDBUFFERARBPROC _glBindBufferARB;
extern PFNGLDELETEBUFFERSARBPROC _glDeleteBuffersARB;
extern PFNGLGENBUFFERSARBPROC _glGenBuffersARB;
extern PFNGLISBUFFERARBPROC _glIsBufferARB;
extern PFNGLBUFFERDATAARBPROC _glBufferDataARB;
extern PFNGLBUFFERSUBDATAARBPROC _glBufferSubDataARB;
extern PFNGLGETBUFFERSUBDATAARBPROC _glGetBufferSubDataARB;
extern PFNGLMAPBUFFERARBPROC _glMapBufferARB;
extern PFNGLUNMAPBUFFERARBPROC _glUnmapBufferARB;
extern PFNGLGETBUFFERPARAMETERIVARBPROC _glGetBufferParameterivARB;
extern PFNGLGETBUFFERPOINTERVARBPROC _glGetBufferPointervARB;

#define GL_ARB_vertex_buffer_object_Define() \
dboolean has_GL_ARB_vertex_buffer_object = false; \
PFNGLBINDBUFFERARBPROC _glBindBufferARB = NULL; \
PFNGLDELETEBUFFERSARBPROC _glDeleteBuffersARB = NULL; \
PFNGLGENBUFFERSARBPROC _glGenBuffersARB = NULL; \
PFNGLISBUFFERARBPROC _glIsBufferARB = NULL; \
PFNGLBUFFERDATAARBPROC _glBufferDataARB = NULL; \
PFNGLBUFFERSUBDATAARBPROC _glBufferSubDataARB = NULL; \
PFNGLGETBUFFERSUBDATAARBPROC _glGetBufferSubDataARB = NULL; \
PFNGLMAPBUFFERARBPROC _glMapBufferARB = NULL; \
PFNGLUNMAPBUFFERARBPROC _glUnmapBufferARB = NULL; \
PFNGLGETBUFFERPARAMETERIVARBPROC _glGetBufferParameterivARB = NULL; \
PFNGLGETBUFFERPOINTERVARBPROC _glGetBufferPointervARB = NULL

#define GL_ARB_vertex_buffer_object_Init() \
has_GL_ARB_vertex_buffer_object = GL_CheckExtension("GL_ARB_vertex_buffer_object"); \
_glBindBufferARB = GL_RegisterProc("glBindBufferARB"); \
_glDeleteBuffersARB = GL_RegisterProc("glDeleteBuffersARB"); \
_glGenBuffersARB = GL_RegisterProc("glGenBuffersARB"); \
_glIsBufferARB = GL_RegisterProc("glIsBufferARB"); \
_glBufferDataARB = GL_RegisterProc("glBufferDataARB"); \
_glBufferSubDataARB = GL_RegisterProc("glBufferSubDataARB"); \
_glGetBufferSubDataARB = GL_RegisterProc("glGetBufferSubDataARB"); \
_glMapBufferARB = GL_RegisterProc("glMapBufferARB"); \
_glUnmapBufferARB = GL_RegisterProc("glUnmapBufferARB"); \
_glGetBufferParameterivARB = GL_RegisterProc("glGetBufferParameterivARB"); \
_glGetBufferPointervARB = GL_RegisterProc("glGetBufferPointervARB")


//
// GL_ARB_texture_env_combine
//
extern dboolean has_GL_ARB_texture_env_combine;

#define GL_ARB_texture_env_combine_Define() \
dboolean has_GL_ARB_texture_env_combine = false;

#define GL_ARB_texture_env_combine_Init() \
has_GL_ARB_texture_env_combine = GL_CheckExtension("GL_ARB_texture_env_combine");

//
// GL_EXT_texture_env_combine
//
extern dboolean has_GL_EXT_texture_env_combine;

#define GL_EXT_texture_env_combine_Define() \
dboolean has_GL_EXT_texture_env_combine = false;

#define GL_EXT_texture_env_combine_Init() \
has_GL_EXT_texture_env_combine = GL_CheckExtension("GL_EXT_texture_env_combine");

//
// GL_EXT_texture_filter_anisotropic
//
extern dboolean has_GL_EXT_texture_filter_anisotropic;

#define GL_EXT_texture_filter_anisotropic_Define() \
dboolean has_GL_EXT_texture_filter_anisotropic = false;

#define GL_EXT_texture_filter_anisotropic_Init() \
has_GL_EXT_texture_filter_anisotropic = GL_CheckExtension("GL_EXT_texture_filter_anisotropic");

//
// GL_ARB_multitexture
//
extern dboolean has_GL_ARB_multitexture;

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
dboolean has_GL_ARB_multitexture = false; \
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
has_GL_ARB_multitexture = GL_CheckExtension("GL_ARB_multitexture"); \
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

//
// GL_EXT_compiled_vertex_array
//
extern dboolean has_GL_EXT_compiled_vertex_array;

extern PFNGLLOCKARRAYSEXTPROC _glLockArraysEXT;
extern PFNGLUNLOCKARRAYSEXTPROC _glUnlockArraysEXT;

#define GL_EXT_compiled_vertex_array_Define() \
dboolean has_GL_EXT_compiled_vertex_array = false; \
PFNGLLOCKARRAYSEXTPROC _glLockArraysEXT = NULL; \
PFNGLUNLOCKARRAYSEXTPROC _glUnlockArraysEXT = NULL

#define GL_EXT_compiled_vertex_array_Init() \
has_GL_EXT_compiled_vertex_array = GL_CheckExtension("GL_EXT_compiled_vertex_array"); \
_glLockArraysEXT = GL_RegisterProc("glLockArraysEXT"); \
_glUnlockArraysEXT = GL_RegisterProc("glUnlockArraysEXT")

#define glLockArraysEXT(first, count) _glLockArraysEXT(first, count)
#define glUnlockArraysEXT() _glUnlockArraysEXT()

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