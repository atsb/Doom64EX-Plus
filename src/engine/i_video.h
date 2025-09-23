// Emacs style mode select   -*- C -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 1993-1997 Id Software, Inc.
// Copyright(C) 2007-2012 Samuel Villarreal
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
//-----------------------------------------------------------------------------

#ifndef __I_VIDEO_H__
#define __I_VIDEO_H__

#include <SDL3/SDL_surface.h>
#include <SDL3/SDL_video.h>


////////////Video///////////////

extern SDL_Surface* screen;
extern SDL_Window* window;
void I_InitVideo(void);
void I_InitScreen(void);
void I_ShutdownVideo(void);
void I_SetMenuCursorMouseRect();
void V_RegisterCvars();

extern float display_scale;
extern int win_px_w, win_px_h;

/*
SDL_WINDOW_BORDERLESS does not work at all on Linux:
- on GNOME and KDE (Plasma) the top bar and taskbar (respectively) are always drawn on top of the window
- on i3 window manager, the game does not draw over the area for the title bar
On the other side, SDL_WINDOW_FULLSCREEN works properly.
Note that on Linux,  the Windows distinction between fullscreen borderless and exclusive fullscreen does not exist.
Always create window as SDL_WINDOW_FULLSCREEN on Linux
*/
#ifndef SDL_PLATFORM_LINUX
#define HAS_FULLSCREEN_BORDERLESS
#endif

#endif
