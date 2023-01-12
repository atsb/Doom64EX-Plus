// Emacs style mode select   -*- C -*-
//-----------------------------------------------------------------------------
//
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
//		OpenGL Shader compiling code.
// 
//-----------------------------------------------------------------------------
#if !defined(_XBOX) && !defined(VITA)
#ifdef __APPLE__ 
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#elif defined SWITCH
#include <GL/gl.h>
#include <GL/glext.h>
#else
#ifdef _WIN32
#include <glad/glad.h>
#else
#include <GL/glu.h>
#include <GL/gl.h>
#endif
#endif

#include "gl_shader.h"

#include "con_console.h"
#include "doomdef.h"
#include <stdio.h>

#if !defined(_XBOX) && !defined(VITA)
GLuint ID;

void GL_LoadShader(const char* vertexShader, const char* fragmentShader) 
{
	//Compile the code.
	unsigned int texture, fragment;
	texture = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(texture, 1, vertexShader, NULL);
	glCompileShader(texture);
	GL_CheckShaderErrors(texture, GL_VERTEX_SHADER);

	fragment = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment, 2, fragmentShader, NULL);
	glCompileShader(fragment);
	GL_CheckShaderErrors(fragment, GL_FRAGMENT_SHADER);

	GL_DestroyShaders(texture, fragment);
}

void GL_CreateProgram(unsigned int Program_ID, unsigned int shader, unsigned int fragment)
{
	//Create The Program.
	Program_ID = glCreateProgram();
	glAttachShader(ID, shader);
	glAttachShader(ID, fragment);
	glLinkProgram(ID);
	GL_CheckShaderErrors(ID, GL_LINK_STATUS);
}

void GL_DestroyShaders(const char* textureShader, const char* fragmentShader)
{
	glDeleteShader(textureShader);
	glDeleteShader(fragmentShader);
}

boolean GL_CheckShaderErrors(unsigned int shader, unsigned int type)
{
	boolean success;
	char log[1024];

	switch(type)
	{
		case GL_VERTEX_SHADER:
		{
			glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
			if (!success)
			{
				glGetShaderInfoLog(shader, 1024, NULL, &log);
				CON_Printf(WHITE, "Failed to load the shader texture, error log: %s", log);
				return success = false;
			}
		}
		
		case GL_FRAGMENT_SHADER:
		{
			glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
			if (!success)
			{
				glGetShaderInfoLog(shader, 1024, NULL, &log);
				CON_Printf(WHITE, "Failed to load the fragment shader, error log: %s", log);
				return success = false;
			}
		}
		
		case GL_LINK_STATUS:
		{
			glGetShaderiv(shader, GL_LINK_STATUS, &success);
			if (!success)
			{
				glGetProgramInfoLog(shader, 1024, NULL, &log);
				CON_Printf(WHITE, "Failed to load the fragment shader, error log: %s", log);
				return success = false;
			}
		}

		default:
		{
			CON_Printf(WHITE, "No errors found");
			return success = true;
		}
	}
	return success;
}
#endif
#endif