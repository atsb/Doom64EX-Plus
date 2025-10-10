// Emacs style mode select   -*- C++ -*-
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
// DESCRIPTION: Texture handling
//
//-----------------------------------------------------------------------------

#include <SDL3/SDL_stdinc.h>

#include "gl_texture.h"
extern int g_psprite_scope;
#include "doomstat.h"
#include "i_png.h"
#include "w_wad.h"
#include "z_zone.h"
#include "gl_main.h"
#include "p_spec.h"
#include "con_console.h"
#include "g_actions.h"
#include "r_main.h"
#include "dgl.h"
#include "i_sectorcombiner.h"

#define GL_MAX_TEX_UNITS    4

extern int game_world_shader_scope;

int         curtexture;
int         cursprite;
int         curtrans;
int         curgfx;

// world textures

int         t_start;
int         t_end;
int         swx_start;
int         numtextures;
dtexture** textureptr;
word* texturewidth;
word* textureheight;
word* texturetranslation;
word* palettetranslation;

// gfx textures

int         g_start;
int         g_end;
int         numgfx;
int* gfx_lumpnum;
dtexture* gfxptr;
word* gfxwidth;
word* gfxorigwidth;
word* gfxheight;
word* gfxorigheight;

// sprite textures

int         s_start;
int         s_end;
dtexture** spriteptr;
int         numsprtex;
word* spritewidth;
float* spriteoffset;
float* spritetopoffset;
word* spriteheight;
word* spritecount;

typedef struct {
	int mode;
	int combine_rgb;
	int combine_alpha;
	int source_rgb[3];
	int source_alpha[3];
	int operand_rgb[3];
	int operand_alpha[3];
	float color[4];
} gl_env_state_t;

static gl_env_state_t gl_env_state[GL_MAX_TEX_UNITS];
static int curunit = -1;
static unsigned char* g_tex_is_masked = NULL;
static unsigned char* g_tex_is_translucent = NULL;
static int            g_tex_num_alloc = 0;

extern void I_ShaderSetTextureSize(int w, int h);
extern void I_ShaderSetUseTexture(int on);
extern void I_ShaderBind(void);
extern void I_ShaderUnBind(void);
extern void I_SectorCombiner_SetCombineRGB(int mode);
extern void I_SectorCombiner_SetCombineAlpha(int mode);
extern void I_SectorCombiner_SetSourceRGB(int slot, int source);
extern void I_SectorCombiner_SetOperandRGB(int slot, int operand);
extern void I_SectorCombiner_SetEnvColor(float r, float g, float b, float a);


// atsb: Added a helper here because we need to do it in like 6 places..  better than pasting.  Helpers aren't in Pascal Case, functions are.
void GL_Env_RGB_Modulate_Alpha_FromTexture(void)
{
	dglTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	dglTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
	dglTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE);
	dglTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
	dglTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_PRIMARY_COLOR);
	dglTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);

	I_SectorCombiner_SetCombineRGB(GL_MODULATE);
	I_SectorCombiner_SetSourceRGB(0, GL_TEXTURE);
	I_SectorCombiner_SetOperandRGB(0, GL_SRC_COLOR);
	I_SectorCombiner_SetSourceRGB(1, GL_PRIMARY_COLOR);
	I_SectorCombiner_SetOperandRGB(1, GL_SRC_COLOR);

	dglTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE);
	dglTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_TEXTURE);
	dglTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);
}

CVAR_EXTERNAL(r_fillmode);
CVAR_CMD(r_texturecombiner, 1) {
	int i;

	curunit = -1;

	for (i = 0; i < GL_MAX_TEX_UNITS; i++) {
		dmemset(&gl_env_state[i], 0, sizeof(gl_env_state_t));
	}
}

//
// CMD_DumpTextures
//

static CMD(DumpTextures) {
	GL_DumpTextures();
}

//
// CMD_ResetTextures
//

static CMD(ResetTextures) {
	GL_ResetTextures();
}

//
// InitWorldTextures
//

static void InitWorldTextures(void) {
	int i = 0;

	t_start = W_GetNumForName("T_START") + 1;
	t_end = W_GetNumForName("T_END") - 1;
	swx_start = -1;
	numtextures = (t_end - t_start) + 1;
	textureptr = (dtexture**)Z_Calloc(sizeof(dtexture*) * numtextures, PU_STATIC, NULL);
	texturetranslation = Z_Calloc(numtextures * sizeof(word), PU_STATIC, NULL);
	palettetranslation = Z_Calloc(numtextures * sizeof(word), PU_STATIC, NULL);
	texturewidth = Z_Calloc(numtextures * sizeof(word), PU_STATIC, NULL);
	textureheight = Z_Calloc(numtextures * sizeof(word), PU_STATIC, NULL);

	for (i = 0; i < numtextures; i++) {
		byte* png;
		int w;
		int h;

		textureptr[i] = (dtexture*)Z_Malloc(1 * sizeof(dtexture), PU_STATIC, 0);

		if (!dstrnicmp(lumpinfo[t_start + i].name, "SWX", 3) && swx_start == -1) {
			swx_start = i;
		}

		texturetranslation[i] = i;
		palettetranslation[i] = 0;

		png = I_PNGReadData(t_start + i, true, true, false, &w, &h, NULL, 0);

		textureptr[i][0] = 0;
		texturewidth[i] = w;
		textureheight[i] = h;

		Z_Free(png);
	}

	CON_DPrintf("%i world textures initialized\n", numtextures);
}

static void GL_WorldTexEnsureCapacity(void)
{
	if (g_tex_num_alloc == numtextures)
		return;
	if (g_tex_num_alloc) {

		if (g_tex_is_masked)
			Z_Free(g_tex_is_masked);

		if (g_tex_is_translucent)
			Z_Free(g_tex_is_translucent);
	}
	g_tex_is_masked = (unsigned char*)Z_Calloc(numtextures, PU_STATIC, NULL);
	g_tex_is_translucent = (unsigned char*)Z_Calloc(numtextures, PU_STATIC, NULL);
	g_tex_num_alloc = numtextures;
}

static void GL_WorldTexClassify(int texnum)
{
	if (!g_tex_is_masked || !g_tex_is_translucent)
		return;

	if (g_tex_is_masked[texnum] || g_tex_is_translucent[texnum])
		return;

	int w = 0, h = 0;
	byte* png = I_PNGReadData(t_start + texnum, false, true, true,
		&w, &h, NULL, palettetranslation[texnum]);
	if (!png || w <= 0 || h <= 0) {
		if (png)
			Z_Free(png);
		return;
	}

	int has0 = 0, has255 = 0, hasMid = 0;
	const unsigned char* a = png + 3;
	const int pixels = w * h;
	for (int i = 0; i < pixels; ++i, a += 4) {
		unsigned char al = *a;
		if (al == 0)
			has0 = 1;
		else if (al == 255)
			has255 = 1;
		else
			hasMid = 1;
		if (hasMid && has0 && has255)
			break;
	}
	g_tex_is_translucent[texnum] = (unsigned char)hasMid;
	g_tex_is_masked[texnum] = (unsigned char)(!hasMid && (has0 || has255));

	Z_Free(png);
}

void GL_WorldTextureEnsureClassified(int texnum)
{
	GL_WorldTexEnsureCapacity();
	if (texnum >= 0 && texnum < numtextures) GL_WorldTexClassify(texnum);
}

int GL_WorldTextureIsTranslucent(int texnum)
{
	GL_WorldTextureEnsureClassified(texnum);
	return (g_tex_is_translucent && texnum >= 0 && texnum < numtextures)
		? g_tex_is_translucent[texnum] : 0;
}

int GL_WorldTextureIsMasked(int texnum)
{
	GL_WorldTextureEnsureClassified(texnum);
	return (g_tex_is_masked && texnum >= 0 && texnum < numtextures)
		? g_tex_is_masked[texnum] : 0;
}

//
// GL_BindWorldTexture
//

extern void I_SectorCombiner_Bind(int useTex, int w, int h);

void GL_BindWorldTexture(int texnum, int* width, int* height) {
	byte* png;
	int w;
	int h;

	if (r_fillmode.value <= 0) {
		return;
	}

	texnum = texturetranslation[texnum];

	if (width) {
		*width = texturewidth[texnum];
	}
	if (height) {
		*height = textureheight[texnum];
	}

	if (t_start <= 0 || t_start >= numlumps ||
		dstrnicmp(lumpinfo[t_start - 1].name, "T_START", 7) != 0) {
		int ts = W_GetNumForName("T_START");
		if (ts >= 0)
			t_start = ts + 1;
	}

	GL_WorldTextureEnsureClassified(texnum);

#define APPLY_ALPHA_MODE_FOR_TEX(_t)                                        \
    do {                                                                          \
        dglDisable(GL_ALPHA_TEST);                                                \
        GL_SetState(GLSTATE_BLEND, false);                                        \
        dglDepthMask(GL_TRUE);                                                    \
        if (g_tex_is_translucent && g_tex_is_translucent[_t]) {                       \
            /* atsb: translucency */                                             \
            GL_SetState(GLSTATE_BLEND, true);                                     \
            dglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);                   \
            dglDepthMask(GL_FALSE);                                                \
            if (!game_world_shader_scope) GL_Env_RGB_Modulate_Alpha_FromTexture();  \
        } else if (g_tex_is_masked && g_tex_is_masked[_t]) {                          \
            /* atsb: (0/255 alpha) */                                    \
            GL_SetState(GLSTATE_BLEND, false);                                    \
            dglEnable(GL_ALPHA_TEST);                                             \
            dglAlphaFunc(GL_GREATER, 0.2f);                                       \
            dglDepthMask(GL_TRUE);                                                \
            dglTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);         \
        } else {                                                                  \
            /* atsb: fully opaque */                                                    \
            GL_SetState(GLSTATE_BLEND, false);                                    \
            dglDisable(GL_ALPHA_TEST);                                            \
            dglDepthMask(GL_TRUE);                                                \
            dglTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);         \
        }                                                                         \
    } while (0)

	/* StevenSYS: Fixes old saves crashing the game, however the textures will be incorrect */
	if (texnum > sizeof(dtexture*) * numtextures) {
		return;
	}

	if (textureptr[texnum][palettetranslation[texnum]]) {
		dglEnable(GL_TEXTURE_2D);
		dglBindTexture(GL_TEXTURE_2D, textureptr[texnum][palettetranslation[texnum]]);
		I_ShaderSetUseTexture(1);
		I_ShaderSetTextureSize(texturewidth[texnum], textureheight[texnum]);
		I_SectorCombiner_Bind(1, texturewidth[texnum], textureheight[texnum]);
		APPLY_ALPHA_MODE_FOR_TEX(texnum);
		dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		if (devparm)
			glBindCalls++;
		return;
	}

	png = I_PNGReadData(t_start + texnum, false, true, true,
		&w, &h, NULL, palettetranslation[texnum]);
	if (!png || w <= 0 || h <= 0 || w > 8192 || h > 8192) {
		GL_BindDummyTexture();
		texturewidth[texnum] = textureheight[texnum] = 1;
		if (width) {
			*width = 1;
			if (height) {
				*height = 1;
			}
		}
		if (png) {
			Z_Free(png);
		}
		curtexture = -1;
		return;
	}

	dglGenTextures(1, &textureptr[texnum][palettetranslation[texnum]]);
	dglBindTexture(GL_TEXTURE_2D, textureptr[texnum][palettetranslation[texnum]]);
	I_ShaderSetUseTexture(1);
	I_ShaderSetTextureSize(texturewidth[texnum], textureheight[texnum]);
	APPLY_ALPHA_MODE_FOR_TEX(texnum);
	dglTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, png);

	dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	GL_CheckFillMode();
	GL_SetTextureFilter();

	curtexture = texnum;

	texturewidth[texnum] = w;
	textureheight[texnum] = h;

	if (width)
		*width = w;
	if (height)
		*height = h;

	Z_Free(png);

	if (devparm)
		glBindCalls++;
}

//
// GL_SetNewPalette
//

void GL_SetNewPalette(int id, byte palID) {
	palettetranslation[id] = palID;
}

//
// SetTextureImage
//

static void SetTextureImage(byte* data, int bits, int* origwidth, int* origheight, int format, int type) {
	dglTexImage2D(
		GL_TEXTURE_2D,
		0,
		format,
		*origwidth,
		*origheight,
		0,
		type,
		GL_UNSIGNED_BYTE,
		data
	);

	GL_CheckFillMode();
	GL_SetTextureFilter();
}

//
// InitGfxTextures
//

static void InitGfxTextures(void) {
	int i, j;

	int* png_indices = (int*)Z_Calloc(sizeof(int) * numlumps, PU_STATIC, NULL);
	int  num_found = 0;

	for (i = numlumps - 1; i >= 0; --i) {
		int len = W_LumpLength(i);
		if (len < 8)
			continue;

		const unsigned char* p = (const unsigned char*)W_CacheLumpNum(i, PU_STATIC);
		if (!p)
			continue;

		if (!(p[0] == 0x89 && p[1] == 0x50 && p[2] == 0x4E && p[3] == 0x47 &&
			p[4] == 0x0D && p[5] == 0x0A && p[6] == 0x1A && p[7] == 0x0A)) {
			continue;
		}

		int dup = 0;
		for (j = 0; j < num_found; ++j) {
			if (!dstrncmp(lumpinfo[png_indices[j]].name, lumpinfo[i].name, 8)) {
				dup = 1;
				break;
			}
		}
		if (!dup)
			png_indices[num_found++] = i;
	}

	for (i = 0, j = num_found - 1; i < j; ++i, --j) {
		int t = png_indices[i]; png_indices[i] = png_indices[j]; png_indices[j] = t;
	}

	g_start = -1;
	g_end = -1;

	numgfx = num_found;
	gfxptr = (dtexture*)Z_Calloc(numgfx * sizeof(dtexture), PU_STATIC, NULL);
	gfxwidth = (int16_t*)Z_Calloc(numgfx * sizeof(int16_t), PU_STATIC, NULL);
	gfxorigwidth = (int16_t*)Z_Calloc(numgfx * sizeof(int16_t), PU_STATIC, NULL);
	gfxheight = (int16_t*)Z_Calloc(numgfx * sizeof(int16_t), PU_STATIC, NULL);
	gfxorigheight = (int16_t*)Z_Calloc(numgfx * sizeof(int16_t), PU_STATIC, NULL);
	gfx_lumpnum = (int*)Z_Calloc(numgfx * sizeof(int), PU_STATIC, NULL);

	for (i = 0; i < numgfx; ++i) {
		int lumpnum = png_indices[i];
		int w = 0, h = 0;
		byte* png = I_PNGReadData(lumpnum, true, true, false, &w, &h, NULL, 0);

		gfxptr[i] = 0;
		gfxwidth[i] = (int16_t)w;
		gfxorigwidth[i] = (int16_t)w;
		gfxorigheight[i] = (int16_t)h;
		gfxheight[i] = (int16_t)h;
		gfx_lumpnum[i] = lumpnum;

		if (png)
			Z_Free(png);
	}

	Z_Free(png_indices);
	CON_DPrintf("%i generic PNG textures initialized (full WAD scan)\n", numgfx);
}

int GL_GetGfxIdForLump(int lump) {
	for (int i = 0; i < numgfx; ++i) {
		if (gfx_lumpnum[i] == lump) {
			return i;
		}
	}
	return -1;
}

//
// GL_BindGfxTexture
//

int GL_BindGfxTexture(const char* name, int alpha) {
	byte* png;
	int lump;
	int width, height;
	int format, type;
	int gfxid;

	lump = W_CheckNumForName(name);
	if (lump < 0) {
		CON_Warnf("GL_BindGfxTexture: '%s' not found\n", name);
		return -1;
	}

	gfxid = GL_GetGfxIdForLump(lump);

	if (gfxid < 0) {
		CON_Warnf("GL_BindGfxTexture: '%s' is not a PNG gfx lump (skipping)\n", name);
		return -1;
	}

	if (gfxid == curgfx) {
		if (alpha) {
			GL_SetState(GLSTATE_BLEND, 1);
			dglDisable(GL_ALPHA_TEST);
			dglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			dglDepthMask(GL_TRUE);
			if (!game_world_shader_scope)
				GL_Env_RGB_Modulate_Alpha_FromTexture();
		}
		else {
			GL_SetState(GLSTATE_BLEND, 0);
			dglDisable(GL_ALPHA_TEST);
			dglDepthMask(GL_TRUE);
			dglTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		}
		I_SectorCombiner_Unbind();
		I_ShaderSetUseTexture(1);
		I_ShaderSetTextureSize(0, 0);
		return gfxid;
	}
	curgfx = gfxid;

	if (gfxptr[gfxid]) {
		dglBindTexture(GL_TEXTURE_2D, gfxptr[gfxid]);
		I_SectorCombiner_Unbind();
		I_ShaderSetUseTexture(1);
		I_ShaderSetTextureSize(0, 0);
		dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
		dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

		// UI state
		if (devparm)
			glBindCalls++;

		if (alpha) {
			GL_SetState(GLSTATE_BLEND, 1);
			dglDisable(GL_ALPHA_TEST);
			dglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			dglDepthMask(GL_TRUE);
			if (!game_world_shader_scope)
				GL_Env_RGB_Modulate_Alpha_FromTexture();
		}
		else {
			GL_SetState(GLSTATE_BLEND, 0);
			dglDisable(GL_ALPHA_TEST);
			dglDepthMask(GL_TRUE);
			dglTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		}

		I_SectorCombiner_Unbind();
		I_ShaderSetUseTexture(1);
		I_ShaderSetTextureSize(0, 0);
		I_SectorCombiner_Unbind();
		I_ShaderSetUseTexture(1);
		I_ShaderSetTextureSize(0, 0);
		return gfxid;
	}

	png = I_PNGReadData(lump, false, true, alpha, &width, &height, NULL, 0);

	dglGenTextures(1, &gfxptr[gfxid]);
	dglBindTexture(GL_TEXTURE_2D, gfxptr[gfxid]);
	I_ShaderSetUseTexture(1);
	dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
	dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	dglPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	format = alpha ? GL_RGBA8 : GL_RGB8;
	type = alpha ? GL_RGBA : GL_RGB;

	SetTextureImage(png, (alpha ? 4 : 3), &width, &height, format, type);
	Z_Free(png);

	gfxwidth[gfxid] = (int16_t)width;
	gfxorigwidth[gfxid] = (int16_t)width;
	gfxorigheight[gfxid] = (int16_t)height;
	gfxheight[gfxid] = (int16_t)height;

	if (alpha) {
		GL_SetState(GLSTATE_BLEND, 1);
		dglDisable(GL_ALPHA_TEST);
		dglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		dglDepthMask(GL_TRUE);
		if (!game_world_shader_scope)
			GL_Env_RGB_Modulate_Alpha_FromTexture();
	}
	else {
		GL_SetState(GLSTATE_BLEND, 0);
		dglDisable(GL_ALPHA_TEST);
		dglDepthMask(GL_TRUE);
		dglTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	}

	if (devparm)
		glBindCalls++;

	I_SectorCombiner_Unbind();
	I_ShaderSetUseTexture(1);
	I_ShaderSetTextureSize(0, 0);
	I_SectorCombiner_Unbind();
	I_ShaderSetUseTexture(1);
	I_ShaderSetTextureSize(0, 0);
	return gfxid;
}

//
// InitSpriteTextures
//

static void InitSpriteTextures(void) {
	int i = 0;
	int j = 0;
	int p = 0;
	int palcnt = 0;
	int offset[2];

	s_start = W_GetNumForName("S_START") + 1;
	s_end = W_GetNumForName("S_END") - 1;
	numsprtex = (s_end - s_start) + 1;
	spritewidth = (word*)Z_Malloc(numsprtex * sizeof(word), PU_STATIC, 0);
	spriteoffset = (float*)Z_Malloc(numsprtex * sizeof(float), PU_STATIC, 0);
	spritetopoffset = (float*)Z_Malloc(numsprtex * sizeof(float), PU_STATIC, 0);
	spriteheight = (word*)Z_Malloc(numsprtex * sizeof(word), PU_STATIC, 0);
	spriteptr = (dtexture**)Z_Malloc(sizeof(dtexture*) * numsprtex, PU_STATIC, 0);
	spritecount = (word*)Z_Calloc(numsprtex * sizeof(word), PU_STATIC, 0);

	for (i = 0; i < numsprtex; i++) {
		spritecount[i]++;

		for (j = 0; j < NUMSPRITES; j++) {
			if (!dstrncmp(lumpinfo[s_start + i].name, sprnames[j], 4)) {
				char palname[9];

				// increase count until a palette is found
				for (p = 1; p < 10; p++) {
					sprintf(palname, "PAL%s%i", sprnames[j], p);
					if (W_CheckNumForName(palname) != -1) {
						palcnt++;
						spritecount[i]++;
					}
					else {
						break;
					}
				}
				break;
			}
		}
	}

	CON_DPrintf("%i sprites initialized\n", numsprtex);
	CON_DPrintf("%i external palettes initialized\n", palcnt);

	for (i = 0; i < numsprtex; i++) {
		byte* png;
		int w;
		int h;
		size_t x;

		spriteptr[i] = (dtexture*)Z_Malloc(spritecount[i] * sizeof(dtexture), PU_STATIC, 0);

		for (x = 0; x < spritecount[i]; x++) {
			spriteptr[i][x] = 0;
		}

		png = I_PNGReadData(s_start + i, true, true, false, &w, &h, offset, 0);

		spritewidth[i] = w;
		spriteheight[i] = h;
		spriteoffset[i] = (float)offset[0];
		spritetopoffset[i] = (float)offset[1];

		Z_Free(png);
	}
}

//
// GL_BindSpriteTexture
//
void GL_BindSpriteTexture(int spritenum, int pal) {
	byte* png;
	int w, h;

	if (r_fillmode.value <= 0)
		return;

	if (pal && pal >= spritecount[spritenum])
		pal = 0;

	if ((spritenum == cursprite) && (pal == curtrans)) {
		GL_SetState(GLSTATE_BLEND, 1);
		dglEnable(GL_TEXTURE_2D);
		dglEnable(GL_ALPHA_TEST);
		dglAlphaFunc(GL_GREATER, 0.2f);
		dglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		dglDepthMask(GL_FALSE);
		if (!game_world_shader_scope)
			GL_Env_RGB_Modulate_Alpha_FromTexture();
		I_ShaderSetUseTexture(1);
		I_ShaderSetTextureSize(spritewidth[spritenum], spriteheight[spritenum]);
		I_SectorCombiner_Bind(1, spritewidth[spritenum], spriteheight[spritenum]);
		return;
	}

	cursprite = spritenum;
	curtrans = pal;

	if (spriteptr[spritenum][pal]) {
		dglBindTexture(GL_TEXTURE_2D, spriteptr[spritenum][pal]);
		GL_SetState(GLSTATE_BLEND, 1);
		dglEnable(GL_ALPHA_TEST);
		dglAlphaFunc(GL_GREATER, 0.2f);
		dglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		dglDepthMask(GL_FALSE);
		dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		if (!game_world_shader_scope)
			GL_Env_RGB_Modulate_Alpha_FromTexture();
		I_ShaderSetUseTexture(1);
		I_ShaderSetTextureSize(spritewidth[spritenum], spriteheight[spritenum]);
		I_SectorCombiner_Bind(1, spritewidth[spritenum], spriteheight[spritenum]);
		if (game_world_shader_scope && !g_psprite_scope) {
			dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		}

		if (devparm)
			glBindCalls++;
		return;
	}

	png = I_PNGReadData(s_start + spritenum, false, true, true, &w, &h, NULL, pal);

	dglGenTextures(1, &spriteptr[spritenum][pal]);
	dglBindTexture(GL_TEXTURE_2D, spriteptr[spritenum][pal]);
	I_ShaderSetUseTexture(1);
	if (game_world_shader_scope && !g_psprite_scope) {
		dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}
	I_ShaderSetTextureSize(spritewidth[spritenum], spriteheight[spritenum]);
	I_SectorCombiner_Bind(1, spritewidth[spritenum], spriteheight[spritenum]);
	I_SectorCombiner_Bind(1, spritewidth[spritenum], spriteheight[spritenum]);
	I_SectorCombiner_Bind(1, spritewidth[spritenum], spriteheight[spritenum]);
	I_SectorCombiner_Bind(1, spritewidth[spritenum], spriteheight[spritenum]);
	dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	SetTextureImage(png, 4, &w, &h, GL_RGBA8, GL_RGBA);
	Z_Free(png);

	GL_SetState(GLSTATE_BLEND, 1);
	dglEnable(GL_ALPHA_TEST);
	dglAlphaFunc(GL_GREATER, 0.2f);
	dglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	dglDepthMask(GL_FALSE);
	if (!game_world_shader_scope)
		GL_Env_RGB_Modulate_Alpha_FromTexture();

	spritewidth[spritenum] = w;
	spriteheight[spritenum] = h;
	I_SectorCombiner_Bind(1, spritewidth[spritenum], spriteheight[spritenum]);
	I_SectorCombiner_Bind(1, spritewidth[spritenum], spriteheight[spritenum]);
	I_SectorCombiner_Bind(1, spritewidth[spritenum], spriteheight[spritenum]);

	if (devparm)
		glBindCalls++;
}

//
// GL_ScreenToTexture
//

dtexture GL_ScreenToTexture(void) {
	dtexture id;
	int width;
	int height;

	dglEnable(GL_TEXTURE_2D);

	dglGenTextures(1, &id);
	dglBindTexture(GL_TEXTURE_2D, id);
	I_ShaderSetUseTexture(1);
	dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	width = GL_PadTextureDims(video_width);
	height = GL_PadTextureDims(video_height);

	dglTexImage2D(
		GL_TEXTURE_2D,
		0,
		GL_RGB8,
		width,
		height,
		0,
		GL_RGB,
		GL_UNSIGNED_BYTE,
		0
	);

	dglCopyTexSubImage2D(
		GL_TEXTURE_2D,
		0,
		0,
		0,
		0,
		0,
		width,
		height
	);

	return id;
}

//
// GL_BindDummyTexture
//

static dtexture dummytexture = 0;

void GL_BindDummyTexture(void) {
	if (dummytexture == 0) {
		//
		// build dummy texture
		//

		byte rgb[48];   // 4x4 RGB texture

		dmemset(rgb, 0xff, 48);

		dglGenTextures(1, &dummytexture);
		dglBindTexture(GL_TEXTURE_2D, dummytexture);
		dglTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, 4, 4, 0, GL_RGB, GL_UNSIGNED_BYTE, rgb);
		dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		I_SectorCombiner_Unbind();
		I_ShaderSetUseTexture(1);
		I_ShaderSetTextureSize(0, 0);

		GL_CheckFillMode();
		GL_SetTextureFilter();
	}
	else {
		dglBindTexture(GL_TEXTURE_2D, dummytexture);
		I_SectorCombiner_Unbind();
		I_ShaderSetUseTexture(1);
		I_ShaderSetTextureSize(0, 0);
	}
}

//
// GL_BindEnvTexture
//

static dtexture envtexture = 0;

void GL_BindEnvTexture(void) {
	rcolor rgb[16];

	if (r_fillmode.value <= 0) {
		return;
	}

	dmemset(rgb, 0xff, sizeof(rcolor) * 16);

	if (envtexture == 0) {
		dglGenTextures(1, &envtexture);
		dglBindTexture(GL_TEXTURE_2D, envtexture);
		I_SectorCombiner_Unbind();
		I_ShaderSetUseTexture(1);
		I_ShaderSetTextureSize(0, 0);
		dglTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, (byte*)rgb);
		dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		GL_CheckFillMode();
		GL_SetTextureFilter();
	}
	else {
		dglBindTexture(GL_TEXTURE_2D, envtexture);
		I_SectorCombiner_Unbind();
		I_ShaderSetUseTexture(1);
		I_ShaderSetTextureSize(0, 0);
	}
}

//
// GL_UpdateEnvTexture
//

static rcolor lastenvcolor = 0;

void GL_UpdateEnvTexture(rcolor color) {
	rcolor env;
	rcolor rgb[16];
	byte* c;
	int i;

	if (!has_GL_ARB_multitexture) {
		return;
	}

	if (r_fillmode.value <= 0) {
		return;
	}

	if (lastenvcolor == color) {
		return;
	}

	dglActiveTextureARB(GL_TEXTURE1_ARB);

	env = color;
	lastenvcolor = color;
	c = (byte*)rgb;

	dmemset(rgb, 0, sizeof(rcolor) * 16);

	for (i = 0; i < 16; i++) {
#ifdef SYS_BIG_ENDIAN
		* c++ = (byte)((env >> 24) & 0xff);
		*c++ = (byte)((env >> 16) & 0xff);
		*c++ = (byte)((env >> 8) & 0xff);
		*c++ = (byte)((env >> 0) & 0xff);
#else
		* c++ = (byte)((env >> 0) & 0xff);
		*c++ = (byte)((env >> 8) & 0xff);
		*c++ = (byte)((env >> 16) & 0xff);
		*c++ = (byte)((env >> 24) & 0xff);
#endif
	}

	dglTexSubImage2D(
		GL_TEXTURE_2D,
		0,
		0,
		0,
		4,
		4,
		GL_RGBA,
		GL_UNSIGNED_BYTE,
		(byte*)rgb
	);

	dglActiveTextureARB(GL_TEXTURE0_ARB);
}

//
// GL_UnloadTexture
//

void GL_UnloadTexture(dtexture* texture) {
	if (*texture != 0) {
		dglDeleteTextures(1, texture);
		*texture = 0;
	}
}

//
// GL_SetTextureUnit
//

void GL_SetTextureUnit(int unit, int enable) {
	if (!has_GL_ARB_multitexture || r_fillmode.value <= 0 || unit > 3)
		return;
	if (curunit == unit)
		return;

	curunit = unit;

	dglActiveTextureARB(GL_TEXTURE0_ARB + unit);
	GL_SetState(GLSTATE_TEXTURE0 + unit, enable);

	if (enable) {
		// atsb: keep the main shader bound for all units so combiner uniforms apply.
		if (game_world_shader_scope)
			I_ShaderBind();
	}
}

//
// GL_SetTextureMode
//

void GL_SetTextureMode(int mode) {
	gl_env_state_t* state = &gl_env_state[curunit];
	if (state->mode == mode)
		return;
	state->mode = mode;
	dglTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, state->mode);
}

//
// GL_SetCombineState
//

void GL_SetCombineState(int combine) {
	gl_env_state_t* state = &gl_env_state[curunit];
	if (state->combine_rgb == combine)
		return;
	state->combine_rgb = combine;
	dglTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, state->combine_rgb);

	I_SectorCombiner_SetCombineRGB(combine);
}

//
// GL_SetCombineStateAlpha
//

void GL_SetCombineStateAlpha(int combine) {
	gl_env_state_t* state = &gl_env_state[curunit];
	if (state->combine_alpha == combine)
		return;
	state->combine_alpha = combine;
	dglTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, state->combine_alpha);
}

//
// GL_SetEnvColor
//

void GL_SetEnvColor(float* param) {
	float* f = (float*)param;
	gl_env_state_t* state = &gl_env_state[curunit];

	if (f == NULL) {
		CON_Warnf("GL_SetEnvColor: passed in NULL for GL_TEXTURE_ENV_COLOR\n");
		return;
	}

	if (state->color[0] == f[0] &&
		state->color[1] == f[1] &&
		state->color[2] == f[2] &&
		state->color[3] == f[3]) {
		return;
	}

	state->color[0] = f[0];
	state->color[1] = f[1];
	state->color[2] = f[2];
	state->color[3] = f[3];

	dglTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, f);
	I_SectorCombiner_SetEnvColor(f[0], f[1], f[2], f[3]);
}

//
// GL_SetCombineSourceRGB
//

void GL_SetCombineSourceRGB(int source, int target) {
	gl_env_state_t* state;

	state = &gl_env_state[curunit];

	if (state->source_rgb[source] == target) {
		return;
	}

	state->source_rgb[source] = target;
	dglTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB + source, state->source_rgb[source]);
	I_SectorCombiner_SetSourceRGB(source, target);
}

//
// GL_SetCombineSourceAlpha
//

void GL_SetCombineSourceAlpha(int source, int target) {
	gl_env_state_t* state;

	state = &gl_env_state[curunit];

	if (state->source_alpha[source] == target) {
		return;
	}

	state->source_alpha[source] = target;
	dglTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA + source, state->source_alpha[source]);
}

//
// GL_SetCombineOperandRGB
//

void GL_SetCombineOperandRGB(int operand, int target) {
	gl_env_state_t* state;

	state = &gl_env_state[curunit];

	if (state->operand_rgb[operand] == target) {
		return;
	}

	state->operand_rgb[operand] = target;
	dglTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB + operand, state->operand_rgb[operand]);
	I_SectorCombiner_SetOperandRGB(operand, target);
}

//
// GL_SetCombineOperandAlpha
//

void GL_SetCombineOperandAlpha(int operand, int target) {
	gl_env_state_t* state;

	state = &gl_env_state[curunit];

	if (state->operand_alpha[operand] == target) {
		return;
	}

	state->operand_alpha[operand] = target;
	dglTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA + operand, state->operand_alpha[operand]);
}

//
// GL_InitTextures
//

void GL_InitTextures(void) {
	CON_DPrintf("--------Initializing textures--------\n", NULL);

	InitWorldTextures();
	InitGfxTextures();
	InitSpriteTextures();

	G_AddCommand("dumptextures", CMD_DumpTextures, 0);
	G_AddCommand("resettextures", CMD_ResetTextures, 0);
}

//
// GL_PadTextureDims
//

#define MAXTEXSIZE    2048
#define MINTEXSIZE    1

int GL_PadTextureDims(int n) {
	int mask = 1;

	while (mask < 0x40000000) {
		if (n == mask || (n & (mask - 1)) == n) {
			return mask;
		}

		mask <<= 1;
	}
	return n;
}

//
// GL_DumpTextures
// Unbinds all textures from memory
//

void GL_DumpTextures(void) {
	int i;
	int j;
	int p;

	for (i = 0; i < numtextures; i++) {
		GL_UnloadTexture(&textureptr[i][0]);

		for (p = 0; p < numanimdef; p++) {
			int lump = W_GetNumForName(animdefs[p].name) - t_start;

			if (lump != i) {
				continue;
			}

			if (animdefs[p].palette) {
				for (j = 1; j < animdefs[p].frames; j++) {
					GL_UnloadTexture(&textureptr[i][j]);
				}
			}
		}
	}

	for (i = 0; i < numsprtex; i++) {
		for (p = 0; p < spritecount[i]; p++) {
			GL_UnloadTexture(&spriteptr[i][p]);
		}
	}

	for (i = 0; i < numgfx; i++) {
		GL_UnloadTexture(&gfxptr[i]);
	}
}

//
// GL_ResetTextures
// Resets the current texture index
//

void GL_ResetTextures(void) {
	curtexture = cursprite = curgfx = -1;
}
