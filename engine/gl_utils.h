// Emacs style mode select   -*- C -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 2007-2012 Samuel Villarreal
// Copyright(C) 2022-2023 André Guilherme
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
// DESCRIPTION:
//		It stores some OpenGL utils here:
// 
//-----------------------------------------------------------------------------
#include <math.h>

#include "gl_main.h"

#include "doomtype.h" //Include all types neccessary.

#ifdef USE_GLFW
#include <GLFW/glfw3.h>
#else
#ifdef __APPLE__
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#else
#include <SDL.h>
#include <SDL_opengl.h>
#endif
#endif

//André: Use this as wrapper to all the functions.
#ifdef USE_GLFW

#define OGL_RED GLFW_RED_BITS
#define OGL_GREEN GLFW_GREEN_BITS
#define OGL_BLUE GLFW_BLUE_BITS
#define OGL_ALPHA GLFW_ALPHA_BITS
#define OGL_BUFFER GLFW_AUX_BUFFERS
#define OGL_DOUBLEBUFFER GLFW_DOUBLEBUFFER
#define OGL_DEPTH GLFW_DEPTH_BITS
#define OGL_STENCIL GLFW_STENCIL_BITS
#define OGL_ACCUM_RED GLFW_ACCUM_RED_BITS 
#define OGL_ACCUM_GREEN GLFW_ACCUM_GREEN_BITS 
#define OGL_ACCUM_BLUE GLFW_ACCUM_BLUE_BITS
#define OGL_ACCUM_ALPHA GLFW_ACCUM_ALPHA_BITS 
#define OGL_STEREO GLFW_STEREO 
#define OGL_MULTISAMPLE_BUFFERS 0//Dummy
#define OGL_MULTISAMPLE 0//Dummy
#define OGL_ACCELERATED 0//Dummy
#define OGL_RETAINED 0//Dummy
#define OGL_MAJOR_VERSION GLFW_VERSION_MAJOR
#define OGL_MINOR_VERSION GLFW_VERSION_MINOR
#define OGL_CONTEXT GLFW_CONTEXT_CREATION_API
#define OGL_CONTEXT_FLAGS 0//Dummy
#define OGL_CONTEXT_PROFILE 0//Dummy
#define OGL_CURRENT_CONTEXT 0//Dummy
#define OGL_FRAMEBUFFER 0//Dummy
#define OGL_CONTEXT_RELEASE GLFW_CONTEXT_RELEASE_BEHAVIOR
#define OGL_CONTEXT_RESET GLFW_LOSE_CONTEXT_ON_RESET
#define OGL_CONTEXT_NO_ERROR GLFW_CONTEXT_NO_ERROR
#define OGL_CONTEXT_FLOATBUFFERS 0//Wip.

#define OGL_WINDOW_HINT(hint, value) glfwWindowHint(hint, value)
#define OGL_DESTROY_WINDOW(window) glfwTerminate(); 
#define OGL_DEFS	\
GLFWwindow *Window	\

#if defined __arm__ || defined __aarch64__ || defined __APPLE__ || defined __LEGACYGL__
#define OGL_VERSION_2_1 GL_VERSION_2_1 ? GL_CLAMP_TO_EDGE : GL_CLAMP
#define OGL_VERSION OGL_VERSION_2_1
#else
#define OGL_VERSION_3_1  GL_VERSION_3_1 ? GL_CLAMP_TO_EDGE : GL_CLAMP
#define OGL_VERSION OGL_VERSION_3_1
#endif
#define OGL_VERSION_DETECTION OGL_VERSION
#else
#ifdef _XBOX
#ifdef _USEFAKEGL09
#define OGL_RED GL_RED
#define OGL_GREEN GL_GREEN
#define OGL_BLUE GL_BLUE
#define OGL_ALPHA GL_ALPHA
#define OGL_BUFFER GLD3D_BUFFER_SIZE
#define OGL_DOUBLEBUFFER GLD3D_BUFFER_SIZE + GLD3D_BUFFER_SIZE
#define OGL_DEPTH GL_DEPTH
#define OGL_STENCIL GL_STENCIL
#define OGL_ACCUM_RED GL_ACCUM + OGL_RED
#define OGL_ACCUM_GREEN GL_ACCUM + OGL_GREEN
#define OGL_ACCUM_BLUE GL_ACCUM + OGL_BLUE
#define OGL_ACCUM_ALPHA GL_ACCUM + OGL_ALPHA
#define OGL_STEREO GL_STEREO
#define OGL_MULTISAMPLE_BUFFERS GLD3D_MULTISAMPLE + OGL_BUFFER
#define OGL_MULTISAMPLE GLD3D_MULTISAMPLE
#define OGL_ACCELERATED 1//Dummy GL_ACCELERATED
#define OGL_RETAINED 1 //Dummy GL_RETAINED_BACKING
#define OGL_MAJOR_VERSION 1
#define OGL_MINOR_VERSION 1
#define OGL_CONTEXT HDC
#define OGL_CONTEXT_FLAGS 1//SDL_GL_CONTEXT_FLAGS
#define OGL_CONTEXT_PROFILE 1//SDL_GL_CONTEXT_PROFILE_MASK
#define OGL_CURRENT_CONTEXT 1//SDL_GL_SHARE_WITH_CURRENT_CONTEXT
#define OGL_FRAMEBUFFER 1//SDL_GL_FRAMEBUFFER_SRGB_CAPABLE
#define OGL_CONTEXT_RELEASE 1//SDL_GL_CONTEXT_RELEASE_BEHAVIOR
#define OGL_CONTEXT_RESET 1//SDL_GL_CONTEXT_RESET_NOTIFICATION
#define OGL_CONTEXT_NO_ERROR 1 //SDL_GL_CONTEXT_NO_ERROR
#define OGL_CONTEXT_FLOATBUFFERS 1 //SDL_GL_FLOATBUFFERS
#define OGL_DOUBLEBUFFER OGL_DOUBLEBUFFER
#define OGL_WINDOW_HINT(attr, value) //Dummy wglSetAttribute(attr, value)
#define OGL_DESTROY_WINDOW(context) wglDeleteContext(context)
#define OGL_DEFS    \
HGLRC    Window

#if defined __arm__ || defined __aarch64__ || defined __APPLE__ || defined __LEGACYGL__
#define OGL_VERSION_2_1 GL_VERSION_2_1 ? GL_CLAMP_TO_EDGE : GL_CLAMP
#define OGL_VERSION OGL_VERSION_2_1
#else
#define OGL_VERSION_3_1  GL_VERSION_3_1 ? GL_CLAMP_TO_EDGE : GL_CLAMP
#define OGL_VERSION OGL_VERSION_3_1
#endif
#define OGL_VERSION_DETECTION OGL_VERSION1
#else
#define OGL_RED GL_RED
#define OGL_GREEN GL_GREEN
#define OGL_BLUE GL_BLUE
#define OGL_ALPHA GL_ALPHA
#define OGL_BUFFER GL_BUFFER
#define OGL_DOUBLEBUFFER SDL_GL_DOUBLEBUFFER
#define OGL_DEPTH SDL_GL_DEPTH_SIZE
#define OGL_STENCIL SDL_GL_STENCIL_SIZE
#define OGL_ACCUM_RED SDL_GL_ACCUM_RED_SIZE
#define OGL_ACCUM_GREEN SDL_GL_ACCUM_GREEN_SIZE
#define OGL_ACCUM_BLUE SDL_GL_ACCUM_BLUE_SIZE
#define OGL_ACCUM_ALPHA SDL_GL_ACCUM_ALPHA_SIZE
#define OGL_STEREO SDL_GL_STEREO
#define OGL_MULTISAMPLE_BUFFERS SDL_GL_MULTISAMPLEBUFFERS
#define OGL_MULTISAMPLE SDL_GL_MULTISAMPLESAMPLES
#define OGL_ACCELERATED SDL_GL_ACCELERATED_VISUAL
#define OGL_RETAINED SDL_GL_RETAINED_BACKING
#define OGL_MAJOR_VERSION SDL_GL_CONTEXT_MAJOR_VERSION
#define OGL_MINOR_VERSION SDL_GL_CONTEXT_MINOR_VERSION
#define OGL_CONTEXT SDL_GL_CONTEXT_EGL
#define OGL_CONTEXT_FLAGS SDL_GL_CONTEXT_FLAGS
#define OGL_CONTEXT_PROFILE SDL_GL_CONTEXT_PROFILE_MASK
#define OGL_CURRENT_CONTEXT SDL_GL_SHARE_WITH_CURRENT_CONTEXT
#define OGL_FRAMEBUFFER SDL_GL_FRAMEBUFFER_SRGB_CAPABLE
#define OGL_CONTEXT_RELEASE SDL_GL_CONTEXT_RELEASE_BEHAVIOR
#define OGL_CONTEXT_RESET SDL_GL_CONTEXT_RESET_NOTIFICATION
#define OGL_CONTEXT_NO_ERROR SDL_GL_CONTEXT_NO_ERROR
#define OGL_CONTEXT_FLOATBUFFERS SDL_GL_FLOATBUFFERS
#define OGL_DOUBLEBUFFER SDL_GL_DOUBLEBUFFER
#define OGL_WINDOW_HINT(attr, value) SDL_GL_SetAttribute(attr, value)
#define OGL_DESTROY_WINDOW(context) SDL_GL_DeleteContext(context)
#define OGL_DEFS    \
SDL_GLContext   Window
#endif

#if defined __arm__ || defined __aarch64__ || defined __APPLE__ || defined __LEGACYGL__
#define OGL_VERSION_2_1 GL_VERSION_2_1 ? GL_CLAMP_TO_EDGE : GL_CLAMP
#define OGL_VERSION OGL_VERSION_2_1
#else
#define OGL_VERSION_3_1  GL_VERSION_3_1 ? GL_CLAMP_TO_EDGE : GL_CLAMP
#define OGL_VERSION OGL_VERSION_3_1
#endif
#define OGL_VERSION_DETECTION OGL_VERSION
#else
#define OGL_RED GL_RED
#define OGL_GREEN GL_GREEN
#define OGL_BLUE GL_BLUE
#define OGL_ALPHA GL_ALPHA
#define OGL_BUFFER GL_BUFFER
#define OGL_DOUBLEBUFFER SDL_GL_DOUBLEBUFFER
#define OGL_DEPTH SDL_GL_DEPTH_SIZE
#define OGL_STENCIL SDL_GL_STENCIL_SIZE
#define OGL_ACCUM_RED SDL_GL_ACCUM_RED_SIZE
#define OGL_ACCUM_GREEN SDL_GL_ACCUM_GREEN_SIZE
#define OGL_ACCUM_BLUE SDL_GL_ACCUM_BLUE_SIZE
#define OGL_ACCUM_ALPHA SDL_GL_ACCUM_ALPHA_SIZE
#define OGL_STEREO SDL_GL_STEREO
#define OGL_MULTISAMPLE_BUFFERS SDL_GL_MULTISAMPLEBUFFERS
#define OGL_MULTISAMPLE SDL_GL_MULTISAMPLESAMPLES
#define OGL_ACCELERATED SDL_GL_ACCELERATED_VISUAL
#define OGL_RETAINED SDL_GL_RETAINED_BACKING
#define OGL_MAJOR_VERSION SDL_GL_CONTEXT_MAJOR_VERSION
#define OGL_MINOR_VERSION SDL_GL_CONTEXT_MINOR_VERSION
#define OGL_CONTEXT SDL_GL_CONTEXT_EGL
#define OGL_CONTEXT_FLAGS SDL_GL_CONTEXT_FLAGS
#define OGL_CONTEXT_PROFILE SDL_GL_CONTEXT_PROFILE_MASK
#define OGL_CURRENT_CONTEXT SDL_GL_SHARE_WITH_CURRENT_CONTEXT
#define OGL_FRAMEBUFFER SDL_GL_FRAMEBUFFER_SRGB_CAPABLE
#define OGL_CONTEXT_RELEASE SDL_GL_CONTEXT_RELEASE_BEHAVIOR
#define OGL_CONTEXT_RESET SDL_GL_CONTEXT_RESET_NOTIFICATION
#define OGL_CONTEXT_NO_ERROR SDL_GL_CONTEXT_NO_ERROR
#define OGL_CONTEXT_FLOATBUFFERS SDL_GL_FLOATBUFFERS
#define OGL_DOUBLE_BUFFER SDL_GL_DOUBLEBUFFER
#define OGL_WINDOW_HINT(attr, value) SDL_GL_SetAttribute(attr, value)
#define OGL_DESTROY_WINDOW(context) SDL_GL_DeleteContext(context)
#define OGL_DEFS    \
SDL_GLContext   Window
#endif

#if defined __arm__ || defined __aarch64__ || defined __APPLE__ || defined __LEGACYGL__
#define OGL_VERSION_2_1 GL_VERSION_2_1 ? GL_CLAMP_TO_EDGE : GL_CLAMP
#define OGL_VERSION OGL_VERSION_2_1
#elif defined(VITA)
#define OGL_VERSION GL_VERSION ? GL_CLAMP_TO_EDGE : GL_CLAMP;
#else
#define OGL_VERSION_3_1  GL_VERSION_3_1 ? GL_CLAMP_TO_EDGE : GL_CLAMP
#define OGL_VERSION OGL_VERSION_3_1
#endif
#define OGL_VERSION_DETECTION OGL_VERSION
#endif

//
// CUSTOM ROUTINES
//
void glSetVertex(vtx_t* vtx);
void glTriangle(int v0, int v1, int v2);
void glDrawGeometry(int count, vtx_t* vtx);
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
void glGetVersion(int major, int minor);
void glDestroyWindow(OGL_DEFS);
#ifdef _WIN32
#define glGetProcAddress wglGetProcAddress
#else
#define glGetProcAddress SDL_GL_GetProcAddress
#endif

#ifndef GLEW
//
// GL_ARB_multitexture
//
extern PFNGLACTIVETEXTUREARBPROC _glActiveTextureARB;

#define glActiveTextureARB(texture) _glActiveTextureARB(texture)
#endif