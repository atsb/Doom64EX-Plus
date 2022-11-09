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
#include <glew.h>
#include "doomtype.h"


/*
	Based on the following references:
	https://www.inf.ufrgs.br/~amaciel/teaching/SIS0384-09-2/exercise9.html
	https://learnopengl.com/Getting-started/Shaders
*/

GLuint ID;
void GL_LoadShader(const char* textureShader, const char* fragmentShader);
void GL_DestroyShaders(const char* textureShader, const char* fragmentShader);
dboolean GL_CheckShaderErrors(GLuint shader, GLenum type);

#endif //__GL_SHADER__H
