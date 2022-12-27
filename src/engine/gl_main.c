// Emacs style mode select   -*- C -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 2007-2012 Samuel Villarreal
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
// DESCRIPTION: OpenGL backend system
//
//-----------------------------------------------------------------------------

#include <math.h>


#ifdef __APPLE__
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#elif defined(VITA)
#include <vitaGL.h>
#else
#include <SDL.h>
#include <SDL_opengl.h>
#endif

#include "doomdef.h"
#include "doomstat.h"
#include "i_video.h"
#include "gl_main.h"
#include "i_system.h"
#include "z_zone.h"
#include "r_main.h"
#include "gl_texture.h"
#include "con_console.h"
#include "m_misc.h"
#include "g_actions.h"
#include "i_sdlinput.h"
#ifdef VITA
#define GL_TEXTURE0_ARB 0x84C0
#endif
int ViewWindowX = 0;
int ViewWindowY = 0;
int ViewWidth = 0;
int ViewHeight = 0;

int gl_max_texture_units;
int gl_max_texture_size;
dboolean gl_has_combiner;

const int8_t* gl_vendor;
const int8_t* gl_renderer;
const int8_t* gl_version;

static float glScaleFactor = 1.0f;

#ifdef VITA
#define GL_FOG_HINT GL_FOG
#endif

dboolean    usingGL = false;
float       max_anisotropic = 0;
dboolean    widescreen = false;

CVAR_EXTERNAL(v_vsync);
CVAR_EXTERNAL(r_filter);
CVAR_EXTERNAL(r_texturecombiner);
CVAR_EXTERNAL(r_anisotropic);
CVAR_EXTERNAL(st_flashoverlay);

//
// CMD_DumpGLExtensions
//

static CMD(DumpGLExtensions) {
	int8_t* string;
	int i = 0;
	int len = 0;

	string = (int8_t*)glGetString(GL_EXTENSIONS);
	len = dstrlen(string);

	for (i = 0; i < len; i++) {
		if (string[i] == 0x20) {
			string[i] = '\n';
		}
	}

	M_WriteTextFile("GL_EXTENSIONS.TXT", string, len);
	CON_Printf(WHITE, "Written GL_EXTENSIONS.TXT\n");
}

// ======================== OGL Extensions ===================================

GL_ARB_multitexture_Define();

//
// FindExtension
//

static dboolean FindExtension(const int8_t* ext) {
	const int8_t* extensions = NULL;
	const int8_t* start;
	const int8_t* where, * terminator;

	// Extension names should not have spaces.
	where = strrchr(ext, ' ');
	if (where || *ext == '\0') {
		return 0;
	}

	extensions = glGetString(GL_EXTENSIONS);

	start = extensions;
	for (;;) {
		where = strstr(start, ext);
		if (!where) {
			break;
		}
		terminator = where + dstrlen(ext);
		if (where == start || *(where - 1) == ' ') {
			if (*terminator == ' ' || *terminator == '\0') {
				return true;
			}
			start = terminator;
		}
	}
	return false;
}

//
// GL_CheckExtension
//

dboolean GL_CheckExtension(const int8_t* ext) {
	if (FindExtension(ext)) {
		CON_Printf(WHITE, "GL Extension: %s = true\n", ext);
		return true;
	}
	else {
		CON_Printf(YELLOW, "GL Extension: %s = false\n", ext);
	}

	return false;
}

//
// GL_RegisterProc
//

void* GL_RegisterProc(const int8_t* address) {
	void* proc = SDL_GL_GetProcAddress(address);

	if (!proc) {
		CON_Warnf("GL_RegisterProc: Failed to get proc address: %s", address);
		return NULL;
	}

	return proc;
}

//
// GL_SetOrtho
//

static byte checkortho = 0;

void GL_SetOrtho(dboolean stretch) {
	float width;
	float height;

	if (checkortho) {
		if (widescreen) {
			if (stretch && checkortho == 2) {
				return;
			}
		}
		else {
			return;
		}
	}

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	if (widescreen && !stretch) {
		const float ratio = (4.0f / 3.0f);
		float fitwidth = ViewHeight * ratio;
		float fitx = (ViewWidth - fitwidth) / 2.0f;

		glViewport(ViewWindowX + (int)fitx, ViewWindowY, (int)fitwidth, ViewHeight);
	}

	width = SCREENWIDTH;
	height = SCREENHEIGHT;

	if (glScaleFactor != 1.0f) {
		width /= glScaleFactor;
		height /= glScaleFactor;
	}

	glOrtho(0, width, height, 0, -1, 1);

	checkortho = (stretch && widescreen) ? 2 : 1;
}

//
// GL_ResetViewport
//

void GL_ResetViewport(void) {
	if (widescreen) {
		glViewport(ViewWindowX, ViewWindowY, ViewWidth, ViewHeight);
	}
}

//
// GL_SetOrthoScale
//

void GL_SetOrthoScale(float scale) {
	glScaleFactor = scale;
	checkortho = 0;
}

//
// GL_GetOrthoScale
//

float GL_GetOrthoScale(void) {
	return glScaleFactor;
}

//
// GL_SwapBuffers
//

void GL_SwapBuffers(void) {
	I_FinishUpdate();
}

//
// GL_GetScreenBuffer
//

byte* GL_GetScreenBuffer(int x, int y, int width, int height) {
	byte* buffer;
	byte* data;
	int i;
	int j;
	int offset1;
	int offset2;
	int pack;
	int col;

	col = (width * 3);
	data = (byte*)Z_Calloc(height * width * 3, PU_STATIC, 0);
	buffer = (byte*)Z_Calloc(col, PU_STATIC, 0);

	//
	// 20120313 villsa - force pack alignment to 1
	//
	glGetIntegerv(GL_PACK_ALIGNMENT, &pack);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glFlush();
	glReadPixels(x, y, width, height, GL_RGB, GL_UNSIGNED_BYTE, data);
	glPixelStorei(GL_PACK_ALIGNMENT, pack);

	//
	// Need to vertically flip the image
	// 20120313 villsa - better method to flip image. uses one buffer instead of two
	//
	for (i = 0; i < height / 2; i++) {
		for (j = 0; j < col; j++) {
			offset1 = (i * col) + j;
			offset2 = ((height - (i + 1)) * col) + j;

			buffer[j] = data[offset1];
			data[offset1] = data[offset2];
			data[offset2] = buffer[j];
		}
	}

	Z_Free(buffer);

	return data;
}

//
// GL_SetTextureFilter
//

void GL_SetTextureFilter(void) {
	if (!usingGL) {
		return;
	}

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (int)r_filter.value == 0 ? GL_LINEAR : GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (int)r_filter.value == 0 ? GL_LINEAR : GL_NEAREST);

	if (GL_EXT_texture_filter_anisotropic) {
		if (r_anisotropic.value) {
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, max_anisotropic);
		}
		else {
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 0);
		}
	}
}

//
// GL_SetDefaultCombiner
//

void GL_SetDefaultCombiner(void) {
	    if (!usingGL) {
        return;
    }
  
	if (GL_ARB_multitexture) {
		GL_SetTextureUnit(1, false);
		GL_SetTextureUnit(2, false);
		GL_SetTextureUnit(3, false);
		GL_SetTextureUnit(0, true);
	}

	GL_CheckFillMode();

	if (r_texturecombiner.value > 0) {
		glTexCombModulate(GL_TEXTURE0_ARB, GL_PRIMARY_COLOR);
	}
	else {
		GL_SetTextureMode(GL_MODULATE);
	}
}

//
// GL_Set2DQuad
//

void GL_Set2DQuad(vtx_t* v, float x, float y, int width, int height,
	float u1, float u2, float v1, float v2, rcolor c) {
	float left, right, top, bottom;

	left = ViewWindowX + x * ViewWidth / video_width;
	right = left + (width * ViewWidth / video_width);
	top = ViewWindowY + y * ViewHeight / video_height;
	bottom = top + (height * ViewHeight / video_height);

	v[0].x = v[2].x = left;
	v[1].x = v[3].x = right;
	v[0].y = v[1].y = top;
	v[2].y = v[3].y = bottom;

	v[0].z = v[1].z = v[2].z = v[3].z = 0.0f;

	v[0].tu = u1;
	v[2].tu = u1;
	v[1].tu = u2;
	v[3].tu = u2;
	v[0].tv = v1;
	v[1].tv = v1;
	v[2].tv = v2;
	v[3].tv = v2;

	glSetVertexColor(v, c, 4);
}

//
// GL_Draw2DQuad
//

void GL_Draw2DQuad(vtx_t* v, dboolean stretch) {
	GL_SetOrtho(stretch);

	glSetVertex(v);
	glTriangle(0, 1, 2);
	glTriangle(3, 2, 1);
	glDrawGeometry(4, v);

	GL_ResetViewport();

	if (devparm) {
		vertCount += 4;
	}
}

//
// GL_SetupAndDraw2DQuad
//

void GL_SetupAndDraw2DQuad(float x, float y, int width, int height,
	float u1, float u2, float v1, float v2, rcolor c, dboolean stretch) {
	vtx_t v[4];

	GL_Set2DQuad(v, x, y, width, height, u1, u2, v1, v2, c);
	GL_Draw2DQuad(v, stretch);
};

//
// GL_SetState
//

static int glstate_flag = 0;

void GL_SetState(int bit, dboolean enable) {
#define TOGGLEGLBIT(flag, bit)                          \
    if(enable && !(glstate_flag & (1 << flag)))         \
    {                                                   \
        glEnable(bit);                                 \
        glstate_flag |= (1 << flag);                    \
    }                                                   \
    else if(!enable && (glstate_flag & (1 << flag)))    \
    {                                                   \
        glDisable(bit);                                \
        glstate_flag &= ~(1 << flag);                   \
    }

	switch (bit) {
	case GLSTATE_BLEND:
		TOGGLEGLBIT(GLSTATE_BLEND, GL_BLEND);
		break;
	case GLSTATE_CULL:
		TOGGLEGLBIT(GLSTATE_CULL, GL_CULL_FACE);
		break;
	case GLSTATE_TEXTURE0:
		TOGGLEGLBIT(GLSTATE_TEXTURE0, GL_TEXTURE_2D);
		break;
	case GLSTATE_TEXTURE1:
		TOGGLEGLBIT(GLSTATE_TEXTURE1, GL_TEXTURE_2D);
		break;
	case GLSTATE_TEXTURE2:
		TOGGLEGLBIT(GLSTATE_TEXTURE2, GL_TEXTURE_2D);
		break;
	case GLSTATE_TEXTURE3:
		TOGGLEGLBIT(GLSTATE_TEXTURE3, GL_TEXTURE_2D);
		break;
	default:
		CON_Warnf("GL_SetState: unknown bit flag: %i\n", bit);
		break;
	}

#undef TOGGLEGLBIT
}

//
// GL_CheckFillMode
//

void GL_CheckFillMode(void) {
	GL_SetState(GLSTATE_TEXTURE0, 1);
}

//
// GL_ClearView
//

void GL_ClearView(rcolor clearcolor) {
	float f[4];

	glGetColorf(clearcolor, f);
	glClearColor(f[0], f[1], f[2], f[3]);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(ViewWindowX, ViewWindowY, ViewWidth, ViewHeight);
	glScissor(ViewWindowX, ViewWindowY, ViewWidth, ViewHeight);
}

//
// GL_GetBool
//

dboolean GL_GetBool(int x) {
	byte b;
	glGetBooleanv(x, &b);

	return (dboolean)b;
}

//
// CalcViewSize
//

static void CalcViewSize(void) {
	ViewWidth = video_width;
	ViewHeight = video_height;

	widescreen = !dfcmp(((float)ViewWidth / (float)ViewHeight), (4.0f / 3.0f));

	ViewWindowX = (video_width - ViewWidth) / 2;

	if (ViewWidth == video_width) {
		ViewWindowY = 0;
	}
	else {
		ViewWindowY = (ViewHeight) / 2;
	}
}

//
void GL_Init(void) {

	gl_vendor = glGetString(GL_VENDOR);
	I_Printf("GL_VENDOR: %s\n", gl_vendor);
	gl_version = glGetString(GL_VERSION);
	I_Printf("GL_VERSION: %s\n", gl_version);
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &gl_max_texture_size);
	I_Printf("GL_MAX_TEXTURE_SIZE: %i\n", gl_max_texture_size);
#ifdef VITA
	glGetIntegerv(GL_MAX_TEXTURE_UNITS, &gl_max_texture_units);
	I_Printf("GL_MAX_TEXTURE_UNITS: %i\n", gl_max_texture_units);
#else
	glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &gl_max_texture_units);
	I_Printf("GL_MAX_TEXTURE_UNITS_ARB: %i\n", gl_max_texture_units);
#endif
	if (gl_max_texture_units <= 2) {
		CON_Warnf("Not enough texture units supported...\n");
	}

	CalcViewSize();

	glViewport(0, 0, video_width, video_height);
	glClearDepth(1.0f);
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	glShadeModel(GL_SMOOTH);
#ifndef VITA	
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
#endif
	glDepthFunc(GL_LEQUAL);
	glAlphaFunc(GL_GEQUAL, ALPHACLEARGLOBAL);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glFogi(GL_FOG_MODE, GL_LINEAR);
	glHint(GL_FOG_HINT, GL_NICEST);
	glEnable(GL_SCISSOR_TEST);
#ifndef VITA
	glEnable(GL_DITHER);
#endif
	GL_SetTextureFilter();
	GL_SetDefaultCombiner();
	GL_ARB_multitexture_Init();
	GL_CheckExtension("GL_EXT_multi_draw_arrays");
	GL_CheckExtension("GL_ARB_vertex_buffer_object"); \
	GL_CheckExtension("GL_EXT_fog_coord");
	GL_CheckExtension("GL_EXT_compiled_vertex_array");
	GL_CheckExtension("GL_ARB_texture_env_combine");
	GL_CheckExtension("GL_EXT_texture_env_combine");
	GL_CheckExtension("GL_EXT_texture_filter_anisotropic");

	if (!GL_ARB_multitexture) {
		CON_Warnf("GL_ARB_multitexture not supported...\n");
	}


	gl_has_combiner = (GL_ARB_texture_env_combine | GL_EXT_texture_env_combine);

	if (!gl_has_combiner) {
		CON_Warnf("Texture combiners not supported...\n");
		CON_Warnf("Setting r_texturecombiner to 0\n");
		CON_CvarSetValue(r_texturecombiner.name, 0.0f);
	}

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
#ifdef VITA
	GL_VERSION ? GL_CLAMP_TO_EDGE : GL_CLAMP;
#else
	GL_VERSION_2_1 ? GL_CLAMP_TO_EDGE : GL_CLAMP;
#endif
	glScaleFactor = 1.0f;

	if (GL_EXT_texture_filter_anisotropic) {
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max_anisotropic);
	}

	usingGL = true;

	G_AddCommand("dumpglext", CMD_DumpGLExtensions, 0);

	SDL_GL_SetSwapInterval(v_vsync.value);
}
