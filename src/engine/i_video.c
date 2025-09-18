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

#include <stdio.h>
#include <SDL3/SDL_opengl.h>

#ifdef SDL_PLATFORM_WIN32
#include <nvapi/nvapi.h>
#include <nvapi/NvApiDriverSettings.h>

#endif
#include "i_video.h"
#include "doomdef.h"
#include "doomstat.h"
#include "i_system.h"
#include "i_sdlinput.h"
#include "gl_main.h"
#include "con_cvar.h"

SDL_Window* window = NULL;
SDL_GLContext   glContext = NULL;

CVAR(r_trishader, 1);
CVAR(v_checkratio, 0);
#ifdef HAS_FULLSCREEN_BORDERLESS
CVAR(v_fullscreen, 0);
#endif


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

static void GetNativeDisplayPixels(int* out_w, int* out_h, SDL_Window* window) {
    *out_w = 0; *out_h = 0;

    SDL_DisplayID display = 0;
    if (window) {
        display = SDL_GetDisplayForWindow(window);
        if (!display) {
            I_Printf("SDL3: SDL_GetDisplayForWindow failed (%s)", SDL_GetError());
        }
    }
    if (!display) {
        display = SDL_GetPrimaryDisplay();
        if (!display) {
            I_Printf("SDL3: SDL_GetPrimaryDisplay failed (%s)", SDL_GetError());
        }
    }

    const SDL_DisplayMode* dm = NULL;
    if (display) {
        dm = SDL_GetDesktopDisplayMode(display);
        if (!dm) {
            I_Printf("SDL3: SDL_GetDesktopDisplayMode failed (%s)", SDL_GetError());
            dm = SDL_GetCurrentDisplayMode(display);
            if (!dm) {
                I_Printf("SDL3: SDL_GetCurrentDisplayMode failed (%s)", SDL_GetError());
            }
        }
    }

    if (dm) {
        *out_w = dm->w;
        *out_h = dm->h;
        return;
    }

    *out_w = 1920;
    *out_h = 1080;
    I_Printf("SDL3: SDL_DisplayMode falling back to %dx%d", *out_w, *out_h);
}

#ifdef SDL_PLATFORM_WIN32

// returns true on error
static boolean NVAPI_IS_ERROR(NvAPI_Status status)
{
    if (status == NVAPI_OK) return false;

    if (status != NVAPI_LIBRARY_NOT_FOUND) {
        NvAPI_ShortString szDesc = { 0 };
        NvAPI_GetErrorMessage(status, szDesc);
        I_Printf("NVAPI error: %s\n", szDesc);
    } // else not a NVIDIA system, do not log error
    return true;
}

// set the NVIDIA Control Panel "Vulkan/OpenGL present method" setting for custom profile "DOOM64EX+" 
// to "Prefer layered on DXGI Swapchain" if "use" is true, or to "Prefer native" otherwise
// "Prefer layered on DXGI Swapchain" is necessary for proper borderless
// "Prefer native" (or "Auto" which is the default) is necessary for proper exclusive fullscreen
static void setUseDXGISwapChainNVIDIA(boolean use) {

    NvAPI_Status status, createAppStatus;
    NvDRSProfileHandle hProfile = 0;
    NvDRSSessionHandle hSession = 0;
    NVDRS_PROFILE profile = { 0 };
    NVDRS_APPLICATION app = { 0 };
    NVDRS_SETTING setting = { 0 };

    if(NVAPI_IS_ERROR(NvAPI_Initialize()) ||
        NVAPI_IS_ERROR(NvAPI_DRS_CreateSession(&hSession)) ||
        NVAPI_IS_ERROR(NvAPI_DRS_LoadSettings(hSession))) goto cleanup;

    profile.version = NVDRS_PROFILE_VER;
    wmemcpy_s(profile.profileName, NVAPI_UNICODE_STRING_MAX, L"DOOM64EX+", 10);
    app.version = NVDRS_APPLICATION_VER;

    status = NvAPI_DRS_FindProfileByName(hSession, profile.profileName, &hProfile);

	switch (status) {
	case NVAPI_OK:
		break;
	case NVAPI_PROFILE_NOT_FOUND:
		if (NVAPI_IS_ERROR(NvAPI_DRS_CreateProfile(hSession, &profile, &hProfile))) goto cleanup;
		break;
	default:
		NVAPI_IS_ERROR(status);
        goto cleanup;
	}

    if (!GetModuleFileNameW(NULL, app.appName, NVAPI_UNICODE_STRING_MAX)) {
        printf("GetModuleFileNameW error: %d\n", GetLastError());
        goto cleanup;
    }

    // try recreate app in case executable has moved to a new location
    // all executable share the same "DOOM64Ex+" profile
    // useful for Release, Debug not in the same location
    createAppStatus = NvAPI_DRS_CreateApplication(hSession, hProfile, &app);
    if (createAppStatus != NVAPI_OK && status == NVAPI_PROFILE_NOT_FOUND) {
        NVAPI_IS_ERROR(createAppStatus);
        goto cleanup;
    } // else app already exist, not an error

    setting.version = NVDRS_SETTING_VER;
    setting.settingId = OGL_CPL_PREFER_DXPRESENT_ID;
    setting.settingType = NVDRS_DWORD_TYPE;
    setting.u32CurrentValue = use ? OGL_CPL_PREFER_DXPRESENT_PREFER_ENABLED : OGL_CPL_PREFER_DXPRESENT_PREFER_DISABLED;

    if(NVAPI_IS_ERROR(NvAPI_DRS_SetSetting(hSession, hProfile, &setting))) goto cleanup;

    NVAPI_IS_ERROR(NvAPI_DRS_SaveSettings(hSession));

cleanup:
    if (hSession) {
        NVAPI_IS_ERROR(NvAPI_DRS_DestroySession(hSession));
    }
}


#endif

//
// I_InitScreen
//

void I_InitScreen(void) {
    unsigned int  flags = 0;
    char          title[256];
	const char *video_driver;

    int native_w = 0, native_h = 0;
    GetNativeDisplayPixels(&native_w, &native_h, window);

    video_width = native_w;
    video_height = native_h;
    video_ratio = (float)video_width / (float)video_height;

    usingGL = false;

    // GL context attributes
	video_driver = SDL_GetCurrentVideoDriver();
	if(!video_driver || !dstreq(video_driver, "wayland")) {
		// do not set these on Wayland as they do not work (bunch of missing extensions)
		// with at least Intel integrated UHD graphics
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	}

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
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

    flags |= SDL_WINDOW_OPENGL | SDL_WINDOW_MAXIMIZED | SDL_WINDOW_INPUT_FOCUS | SDL_WINDOW_MOUSE_FOCUS | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;

#ifdef HAS_FULLSCREEN_BORDERLESS
    flags |= (int)v_fullscreen.value ? SDL_WINDOW_FULLSCREEN : SDL_WINDOW_BORDERLESS;
#else
    flags |= SDL_WINDOW_FULLSCREEN;
#endif

#ifdef SDL_PLATFORM_WIN32
    /* if the window is borderless, the NVIDIA driver will make it fullscreen after the first SDL_GL_SwapWindow call :
       https://github.com/libsdl-org/SDL/issues/12791
       Enabling use of the DXGI Swap Chain with NvAPI makes it proper borderless, with instant Alt-Tab, 
       overlay popups displaying, working Auto-HDR...
     */
    setUseDXGISwapChainNVIDIA(flags & SDL_WINDOW_BORDERLESS);
#endif

    if (glContext) { SDL_GL_DestroyContext(glContext); glContext = NULL; }
    if (window) { SDL_DestroyWindow(window); window = NULL; }

    sprintf(title, "Doom64EX+ - Version Date: %s", version_date);
    window = SDL_CreateWindow(title, video_width, video_height, flags);
    if (!window) {
        I_Error("I_InitScreen: Failed to create window");
        return;
    }

    if (flags & SDL_WINDOW_FULLSCREEN) {
        SDL_DisplayID displayid = SDL_GetDisplayForWindow(window);
        if (!displayid) displayid = SDL_GetPrimaryDisplay();

        const SDL_DisplayMode *desk = displayid ? SDL_GetDesktopDisplayMode(displayid) : NULL;
        SDL_DisplayMode disp_mode = {0};
        if (desk) {
            if (!SDL_GetClosestFullscreenDisplayMode(displayid, desk->w, desk->h, 0.0f, false, &disp_mode)) {
                disp_mode.displayID = displayid;
                disp_mode.w = video_width;
                disp_mode.h = video_height;
                disp_mode.refresh_rate = 0.0f;
            }
        } else {
            disp_mode.displayID = displayid;
            disp_mode.w = video_width;
            disp_mode.h = video_height;
            disp_mode.refresh_rate = 0.0f;
        }

        SDL_SetWindowFullscreenMode(window, &disp_mode);
        SDL_SetWindowFullscreen(window, true);
        SDL_SyncWindow(window);
    }

#ifdef SDL_PLATFORM_LINUX
    // NVIDIA Linux specific: reduces input lag with vsync; harmless elsewhere
    SDL_Environment* env = SDL_GetEnvironment();
    if (env) {
        if (!SDL_SetEnvironmentVariable(env, "__GL_MaxFramesAllowed", "1", false)) {
            I_Printf("SDL_SetEnvironmentVariable failed (%s)", SDL_GetError());
        }
    }
    else {
        I_Printf("SDL_GetEnvironment failed (%s)", SDL_GetError());
    }
#endif

    glContext = SDL_GL_CreateContext(window);
    if (!glContext) {
        I_Error("I_InitScreen: Failed to create OpenGL context");
        return;
    }
    if(!SDL_GL_MakeCurrent(window, glContext)) {
		I_Error("I_InitScreen: Failed to make OpenGL context current");
        return;
	}
    SDL_GetWindowSizeInPixels(window, &win_px_w, &win_px_h);
    GL_OnResize(win_px_w, win_px_h);
    {
        SDL_DisplayID displayid = SDL_GetDisplayForWindow(window);
        if (displayid) {
            float f = SDL_GetDisplayContentScale(displayid);
            if (f != 0)	display_scale = f;
            else I_Printf("SDL_GetDisplayContentScale failed (%s)", SDL_GetError());
        }
        else {
            I_Printf("SDL_GetDisplayForWindow failed (%s)", SDL_GetError());
        }
    }
    SDL_GL_SetSwapInterval((int)v_vsync.value);
    {
        glViewport(0, 0, win_px_w, win_px_h);
        glScissor(0, 0, win_px_w, win_px_h);
        glClearColor(0.f, 0.f, 0.f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    SDL_ShowWindow(window);
    SDL_GL_SwapWindow(window);
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

#ifdef SDL_PLATFORM_LINUX

	/*
	  On Wayland, this hint is needed for proper expected window size and scaling.
	  Without this hint, on a 4K display with 2x desktop scaling, reported fullscreen window size
	  would be 1920x1080 with SDL_GetDisplayContentScale() = 1 (instead of expected 3840x2160, SDL_GetDisplayContentScale() = 2
	  Must be done before SDL_Init()
	*/
	
	SDL_SetHintWithPriority(SDL_HINT_VIDEO_WAYLAND_SCALE_TO_DISPLAY, "1", SDL_HINT_OVERRIDE);

#endif

    if (!SDL_Init(SDL_INIT_VIDEO)) {
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
    CON_CvarRegister(&r_trishader);
    CON_CvarRegister(&v_checkratio);
    CON_CvarRegister(&v_vsync);
#ifdef HAS_FULLSCREEN_BORDERLESS
    CON_CvarRegister(&v_fullscreen);
#endif
}

