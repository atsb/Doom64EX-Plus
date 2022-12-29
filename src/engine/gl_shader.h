// Emacs style mode select   -*- C -*-
//-----------------------------------------------------------------------------
//
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

#ifndef __GL_SHADER__H
#define __GL_SHADER__H

#include "doomtype.h"
#ifndef _XBOX
#ifdef _WIN32
#include <glew.h>
#endif
#ifdef __APPLE__ 
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#elif defined SWITCH
#include <GL/gl.h>
#include <GL/glext.h>
#elif defined(VITA)

#else
#include <GL/glu.h>
#include <GL/gl.h>
#endif
#endif


/*
	Based on the following references:
	https://www.inf.ufrgs.br/~amaciel/teaching/SIS0384-09-2/exercise9.html
	https://learnopengl.com/Getting-started/Shaders
*/
void GL_LoadShader(const char* textureShader, const char* fragmentShader);
void GL_DestroyShaders(const char* textureShader, const char* fragmentShader);
boolean GL_CheckShaderErrors(uint32_t shader, uint32_t type);
void GL_CreateProgram(uint32_t Program_ID, uint32_t shader, uint32_t fragment);

#endif //__GL_SHADER__H
