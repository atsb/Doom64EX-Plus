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
#include "m_misc.h"
#include "doomdef.h"
#include "doomstat.h"
#include "i_system.h"
#include "i_video.h"
#include "i_sdlinput.h"
#include "d_main.h"
#include "con_console.h"
#include "gl_utils.h"
const char version_date[] = __DATE__;

SDL_Window* window;

OGL_DEFS;

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

#ifdef USE_GLFW
void I_ResizeCallback(OGL_DEFS, int width, int height)
{
	glViewport(0, 0, width, height);
}
#endif

#ifdef USE_IMGUI
ImGuiIO* io;
ImGuiContext* ctx;
#endif
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
		newwidth = atoi(myargv[p + 1]);
	}

	p = M_CheckParm("-height");
	if (p && p < myargc - 1) {
		newheight = atoi(myargv[p + 1]);
	}

	if (newwidth && newheight) {
		video_width = newwidth;
		video_height = newheight;
		CON_CvarSetValue(v_width.name, (float)video_width);
		CON_CvarSetValue(v_height.name, (float)video_height);
	}

	usingGL = false;
#ifdef LEGACY_DETECTION
#if defined __arm__ || defined __aarch64__ || defined __APPLE__ || defined __LEGACYGL__
	glGetVersion(2, 1);
#else
	glGetVersion(3, 1);
#endif
#else
	OGL_WINDOW_HINT(OGL_CONTEXT_PROFILE_MASK, OGL_CONTEXT_PROFILE_CORE);
#endif
	OGL_WINDOW_HINT(OGL_RED, 0);
	OGL_WINDOW_HINT(OGL_GREEN, 0);
	OGL_WINDOW_HINT(OGL_BLUE, 0);
	OGL_WINDOW_HINT(OGL_ALPHA, 0);
	OGL_WINDOW_HINT(OGL_STENCIL, 0);
	OGL_WINDOW_HINT(OGL_ACCUM_RED, 0);
	OGL_WINDOW_HINT(OGL_ACCUM_GREEN, 0);
	OGL_WINDOW_HINT(OGL_ACCUM_BLUE, 0);
	OGL_WINDOW_HINT(OGL_ACCUM_ALPHA, 0);
	OGL_WINDOW_HINT(OGL_BUFFER, 24);
	OGL_WINDOW_HINT(OGL_DEPTH, 24);
	OGL_WINDOW_HINT(OGL_DOUBLEBUFFER, 1);

#ifdef USE_GLFW
	//int monitor_count, video_mode_count;
	//GLFWmonitor** monitors;
	//monitors = glfwGetMonitors(&monitor_count);
	//GLFWvidmode* modes = glfwGetVideoModes(monitors, &video_mode_count);
	//modes->width = video_width;
	//modes->height = video_height;
	//modes->refreshRate = video_ratio;

	if (glfwInit() < 0)
	{
		I_Error("I_InitScreen: Failed to create glfw");
		glDestroyWindow(Window);
		return;
	}

	sprintf(title, "Doom64EX+ - Version Date: %s", version_date);
	Window = glfwCreateWindow(video_width, video_height, title, NULL, NULL);
	if(!Window)
	{
		I_Error("Failed to create GLFW window");
		glDestroyWindow(Window);
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, I_ResizeCallback);

	while(!glfwWindowShouldClose(window))
	{
		glfwSwapInterval(v_vsync.value);
		glfwPollEvents();//Poll events.

		I_StartTic();
	}

#else
	flags |= SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_INPUT_FOCUS | SDL_WINDOW_MOUSE_FOCUS;

	if (InWindow) {
		flags |= SDL_WINDOW_ALLOW_HIGHDPI;
	}

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

	if ((Window = SDL_GL_CreateContext(window)) == NULL) {
		I_Error("I_InitScreen: Failed to create OpenGL context");
		return;
	}
#endif
#ifndef DONT_USE_GLAD
	if (gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress) < 0)
	{
		I_Error("Failed to load glad");
	}
#endif	
#ifdef USE_IMGUI
	//Andr�: Adding the context
	io = igGetIO();
	ctx = igCreateContext(NULL);
#ifdef __LEGACYGL__
	ImGuiImplGL();
#else
	ImGuiImplGL("#version 330 core"); 
#endif
#endif
}

//
// I_ShutdownVideo
//

void I_ShutdownVideo(void) {

	glDestroyWindow(Window);
#ifndef USE_GLFW
	if (window) {
		SDL_DestroyWindow(window);
		window = NULL;
	}
#endif
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
		I_Error("ERROR - Failed to initialize SDL, ERROR: %", SDL_GetError());
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
