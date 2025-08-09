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
//
// DESCRIPTION:
//    SDL Stuff
//
//-----------------------------------------------------------------------------

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>

#include "m_misc.h"
#include "doomdef.h"
#include "doomstat.h"
#include "i_system.h"
#include "i_video.h"
#include "i_sdlinput.h"
#include "d_main.h"
#include "gl_main.h"

SDL_Window* window = NULL;
SDL_GLContext   glContext = NULL;

CVAR(v_checkratio, 0);
CVAR(v_windowed, 1);
CVAR(v_windowborderless, 0);

CVAR_CMD(v_vsync, 1) {
    SDL_GL_SetSwapInterval((int)v_vsync.value);
}

float display_scale = 1.0f;
SDL_Surface* screen;
int video_width;
int video_height;
float video_ratio;
boolean window_focused;

float mouse_x = 0.0f;
float mouse_y = 0.0f;

int win_px_w = 0;
int win_px_h = 0;

void GL_OnResize(int w, int h);

static void NVidiaGLFinishConditional(void) {
    const char* vendor = (const char*)glGetString(GL_VENDOR);

    if (vendor) {
        if (strcmp(vendor, "NVIDIA Corporation") != 0) {
            glFinish();
            I_Printf("glFinish: Called for: %s\n", vendor);
        }
        else {
            I_Printf("Vendor is NVIDIA! Skipping glFinish.\n");
        }
    }
    else {
        I_Printf("Error: Could not retrieve vendor.\n");
    }
}

static void GetNativeDisplayPixels(int* out_w, int* out_h) {
    *out_w = 0; *out_h = 0;

    SDL_DisplayID primary = SDL_GetPrimaryDisplay();
    if (!primary) {
        I_Printf("SDL_GetPrimaryDisplay failed (%s)", SDL_GetError());
    }

    SDL_DisplayMode mode;
    memset(&mode, 0, sizeof(mode));

    if (primary && SDL_GetCurrentDisplayMode(primary) == 0) {
        *out_w = mode.w;
        *out_h = mode.h;
    }
    else {
        *out_w = 1920;
        *out_h = 1080;
        I_Printf("SDL_GetCurrentDisplayMode failed (%s); defaulting to %dx%d",
            SDL_GetError(), *out_w, *out_h);
    }
}

//
// I_InitScreen
//

void I_InitScreen(void) {
    unsigned int  flags = 0;
    char    title[256];
    SDL_DisplayID displayid;

    InWindow = (int)v_windowed.value;
    InWindowBorderless = (int)v_windowborderless.value;

    int native_w = 0, native_h = 0;
    GetNativeDisplayPixels(&native_w, &native_h);

    video_width = native_w;
    video_height = native_h;
    video_ratio = (float)video_width / (float)video_height;

    usingGL = false;

#if defined __arm__ || defined __aarch64__ || defined __APPLE__ || defined __LEGACYGL__
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
#else
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#endif
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_ACCUM_RED_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_ACCUM_GREEN_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_ACCUM_BLUE_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_ACCUM_ALPHA_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    flags |= SDL_WINDOW_OPENGL | SDL_WINDOW_INPUT_FOCUS | SDL_WINDOW_MOUSE_FOCUS;

#ifndef __APPLE__
    flags |= SDL_WINDOW_HIGH_PIXEL_DENSITY;
#endif

    if (!InWindow) {
        flags |= SDL_WINDOW_FULLSCREEN;
    }

    if (InWindowBorderless) {
        flags |= SDL_WINDOW_BORDERLESS;
    }

    if (glContext) {
        SDL_GL_DestroyContext(glContext);
        glContext = NULL;
    }

    if (window) {
        SDL_DestroyWindow(window);
        window = NULL;
    }

    sprintf(title, "Doom64EX+ - Version Date: %s", version_date);
    window = SDL_CreateWindow(title,
        video_width,
        video_height,
        flags);

    if (window == NULL) {
        I_Error("I_InitScreen: Failed to create window");
        return;
    }

#ifdef __linux__
    // NVIDIA Linux specific: fixes input lag with vsync on
    putenv("__GL_MaxFramesAllowed=1");
#endif

    if ((glContext = SDL_GL_CreateContext(window)) == NULL) {
        I_Error("I_InitScreen: Failed to create OpenGL context");
        return;
    }

    SDL_GetWindowSizeInPixels(window, &win_px_w, &win_px_h);
    GL_OnResize(win_px_w, win_px_h);

    displayid = SDL_GetDisplayForWindow(window);
    if (displayid) {
        float f = SDL_GetDisplayContentScale(displayid);
        if (f != 0) {
            display_scale = f;
        }
        else {
            I_Printf("SDL_GetDisplayContentScale failed (%s)", SDL_GetError());
        }
    }
    else {
        I_Printf("SDL_GetDisplayForWindow failed (%s)", SDL_GetError());
    }

    SDL_GL_SetSwapInterval((int)v_vsync.value);

    SDL_GL_SwapWindow(window);

    NVidiaGLFinishConditional();

    SDL_HideCursor();
}

//
// I_ShutdownVideo
//

void I_ShutdownVideo(void) {
    if (glContext) {
        SDL_GL_DestroyContext(glContext);
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

    if (!SDL_Init(f)) {
        I_Error("ERROR - Failed to initialize SDL");
        return;
    }

    I_StartTic();
    I_InitScreen();
}

//
// V_RegisterCvars
//

void V_RegisterCvars(void) {
    CON_CvarRegister(&v_checkratio);
    CON_CvarRegister(&v_windowed);
    CON_CvarRegister(&v_windowborderless);
    CON_CvarRegister(&v_vsync);
}
