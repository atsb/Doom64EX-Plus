// Emacs style mode select   -*- C -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 2005 Simon Howard
// Copyright(C) 2007-2014 Samuel Villarreal
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
//
// DESCRIPTION:
//    SDL Stuff
//
//-----------------------------------------------------------------------------

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#ifdef __OpenBSD__
#include <SDL.h>
#include <SDL_opengl.h>
#else
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#endif

#include "m_misc.h"
#include "doomdef.h"
#include "doomstat.h"
#include "i_system.h"
#include "i_video.h"
#include "i_sdlinput.h"
#include "d_main.h"
#include "gl_main.h"

SDL_Window* window;
SDL_GLContext   glContext;

#if defined(_WIN32) && defined(USE_XINPUT)
#include "i_xinput.h"
#endif

CVAR(v_width, 640);
CVAR(v_height, 480);
CVAR(v_windowed, 1);
CVAR(v_windowborderless, 0);

SDL_Surface* screen;
int video_width;
int video_height;
float video_ratio;
boolean window_focused;

int mouse_x = 0;
int mouse_y = 0;

//
// I_InitScreen
//

void I_InitScreen(void) {
	int     newwidth;
	int     newheight;
	int     p;
	unsigned int  flags = 0;
	char    title[256];

	InWindow = (int)v_windowed.value;
	InWindowBorderless = (int)v_windowborderless.value;
	video_width = (int)v_width.value;
	video_height = (int)v_height.value;
	video_ratio = (float)video_width / (float)video_height;

	if (M_CheckParm("-borderless")) {
		InWindowBorderless = 1;
	}
	if (M_CheckParm("-window")) {
		InWindow = 1;
	}
	if (M_CheckParm("-fullscreen")) {
		InWindow = 0;
	}

	newwidth = newheight = 0;

	p = M_CheckParm("-width");
	if (p && p < myargc - 1) {
		newwidth = datoi(myargv[p + 1]);
	}

	p = M_CheckParm("-height");
	if (p && p < myargc - 1) {
		newheight = datoi(myargv[p + 1]);
	}

	if (newwidth && newheight) {
		video_width = newwidth;
		video_height = newheight;
		CON_CvarSetValue(v_width.name, (float)video_width);
		CON_CvarSetValue(v_height.name, (float)video_height);
	}

	usingGL = false;

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 0);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 0);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 0);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 0);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);
	SDL_GL_SetAttribute(SDL_GL_ACCUM_RED_SIZE, 0);
	SDL_GL_SetAttribute(SDL_GL_ACCUM_GREEN_SIZE, 0);
	SDL_GL_SetAttribute(SDL_GL_ACCUM_BLUE_SIZE, 0);
	SDL_GL_SetAttribute(SDL_GL_ACCUM_ALPHA_SIZE, 0);
	SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	flags |= SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_INPUT_FOCUS | SDL_WINDOW_MOUSE_FOCUS | SDL_WINDOW_ALLOW_HIGHDPI;

	if (!InWindow) {
		flags |= SDL_WINDOW_FULLSCREEN;
	}

	if (InWindowBorderless) {
		flags |= SDL_WINDOW_BORDERLESS;
	}

	sprintf(title, "Doom64EX+ - Version Date: %s", version_date);
	window = SDL_CreateWindow(title,
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		video_width,
		video_height,
		flags);

	if (window == NULL) {
		I_Error("I_InitScreen: Failed to create window");
		return;
	}

	if ((glContext = SDL_GL_CreateContext(window)) == NULL) {
		I_Error("I_InitScreen: Failed to create OpenGL context");
		return;
	}
}

//
// I_ShutdownVideo
//

void I_ShutdownVideo(void) {
	if (glContext) {
		SDL_GL_DeleteContext(glContext);
		glContext = NULL;
	}

	if (window) {
		SDL_DestroyWindow(window);
		window = NULL;
	}

	SDL_Quit();
}

//
// I_InitVideo
//

void I_InitVideo(void) {
	unsigned int f = SDL_INIT_VIDEO;

#ifdef _DEBUG
	f |= SDL_INIT_NOPARACHUTE;
#endif

	if (SDL_Init(f) < 0) {
		I_Error("ERROR - Failed to initialize SDL");
		return;
	}

	SDL_ShowCursor(SDL_DISABLE);
	I_StartTic();
	I_InitScreen();
}

//
// V_RegisterCvars
//

void V_RegisterCvars(void) {
	CON_CvarRegister(&v_width);
	CON_CvarRegister(&v_height);
	CON_CvarRegister(&v_windowed);
	CON_CvarRegister(&v_windowborderless);
}
