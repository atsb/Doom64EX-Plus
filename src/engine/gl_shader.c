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

#include "gl_shader.h"
#include <stdio.h> 
#include <fnctl.h> //open();, read(); and etc
GLuint ID;

void GL_LoadShader(const char* textureShader, const char* fragmentShader) 
{
	const char* textureShaderCode;
	const char* fragmentShaderCode;
	const char* GLSL_DIR = "GLSL/";
	//Open the dir GLSL/.
	int fd = open(GLSL_DIR, O_RWDIR);
        read(fd, textureShader, sizeof(textureShader));
	open(fd, fragmentShader, sizeof(fragmentShaderL));
       //TODO: Add switch case for detect errors in opening file.
       //
	//Open the shader file.
	FILE* textureShaderFile = fopen(textureShader, "rb");
	FILE* fragmentShaderFile = fopen(fragmentShader, "rb");

	//read the shaders code
	fread(textureShaderCode, sizeof(textureShaderCode), 1, textureShaderFile);
	fread(fragmentShaderCode, sizeof(fragmentShaderCode), 2, fragmentShaderFile);

	if(!textureShaderFile)
	{
		printf("Failed to load the texture shader, error: %lu", glGetError());
         return fclose(textureShaderFile);
	}
	else
	{
	   
           fseek(textureShaderfile, sizeof(textureShaderCode, SEEK_SET);
	   fssek(textureShaderFile, sizeof(textireShaderFile, SEEK_END);
	     return ftell(textureShaderFile);
	
	}
	if(!fragmentShaderFile)
	{
		printf("Failed to load the fragment shader, error: %lu", glGetError());
		return fclose(fragmentShaderFile);
	}

	else
	{
           fseek(fragentShaderFile, sizeof(fragmentShaderCode, SEEK_SET);
	   fseek(fragmentShaderFile, sizeof(fragmentShaderCode), SEEK_END);
	   return ftell(fragmemtShaderFile);
	}
	
	const char* tShaderCode = textureShaderCode;
	const char* fShaderCode = fragmentShaderCode;

	//Compile the code.
	uint32 texture, fragment;
	texture = glCreateShader(GL_TEXTURE_SHADER_NV);
	glShaderSource(texture, 1, tShaderCode, NULL);
	glCompileShader(texture);
	GL_CheckShaderErrors(texture, GL_TEXTURE_SHADER_NV);

	fragment = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment, 2, fShaderCode, NULL);
	glCompileShader(fragment);
	GL_CheckShaderErrors(fragment, GL_FRAGMENT_SHADER);

	GL_DestroyShaders(texture, fragment);

	//Create The Program.
	ID = glCreateProgram();
	glAttachShader(ID, texture);
	glAttachShader(ID, fragment);
	glLinkProgram(ID);
	glUseProgram(ID);
	glDeleteProgram(ID);
	GL_CheckShaderErrors(ID, GL_PROGRAM);
}

void GL_DestroyShaders(const char* textureShader, const char* fragmentShader)
{
	glDeleteShader(textureShader);
	glDeleteShader(fragmentShader);
}

dboolean GL_CheckShaderErrors(GLuint shader, GLenum type)
{
	dboolean success;
	char log[1024];

	switch(type)
	{
		case GL_TEXTURE_SHADER_NV:
		{
			glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
			if (!success)
			{
				glGetShaderInfoLog(shader, 1024, NULL, &log);
				printf("Failed to load the shader texture, error log: %s", log);
				return success = false;
			}
		}
		
		case GL_FRAGMENT_SHADER:
		{
			glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
			if (!success)
			{
				glGetShaderInfoLog(shader, 1024, NULL, &log);
				printf("Failed to load the fragment shader, error log: %s", log);
				return success = false;
			}
		}
		
		case GL_PROGRAM:
		{
			glGetShaderiv(shader, GL_LINK_STATUS, &success);
			if (!success)
			{
				glGetProgramInfoLog(shader, 1024, NULL, &log);
				printf("Failed to load the fragment shader, error log: %s", log);
				return success = false;
			}
		}

		default:
		{
			printf("No errors found hehe");
			return success = true;
		}
	}
	return success;
}
