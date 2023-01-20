// Emacs style mode select   -*- C -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 2007-2012 Samuel Villarreal
// Copyright(C) 2022-2023 André Gulherme
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
// DESCRIPTION: Texture handling
//
//-----------------------------------------------------------------------------
//Remove this later:
#define GL_TEXTURE0_ARB				0x84C0
#define GL_TEXTURE1_ARB				0x84C1

#include "gl_utils.h"
#include "gl_shader.h"
#include "doomstat.h"
#include "r_main.h"
#include "r_things.h"
#include "r_lights.h"
#include "i_png.h"
#include "i_system.h"
#include "w_wad.h"
#include "z_zone.h"
#include "gl_texture.h"
#include "p_local.h"
#include "con_console.h"
#include "g_actions.h"

#define GL_MAX_TEX_UNITS    4

int         curtexture;
int         cursprite;
int         curtrans;
int         curgfx;

// world textures

int         t_start;
int         t_end;
int         swx_start;
int         numtextures;
unsigned int** textureptr;
unsigned short* texturewidth;
unsigned short* textureheight;
unsigned short* texturetranslation;
unsigned short* palettetranslation;

// gfx textures

int         g_start;
int         g_end;
int         numgfx;
unsigned int* gfxptr;
unsigned short* gfxwidth;
unsigned short* gfxorigwidth;
unsigned short* gfxheight;
unsigned short* gfxorigheight;

// sprite textures

int         s_start;
int         s_end;
unsigned int** spriteptr;
int         numsprtex;
unsigned short* spritewidth;
float* spriteoffset;
float* spritetopoffset;
unsigned short* spriteheight;
unsigned short* spritecount;

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
	curtexture = cursprite = curgfx = -1;
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
	textureptr = (unsigned int**)Z_Calloc(sizeof(unsigned int*) * numtextures, PU_STATIC, NULL);
	texturetranslation = Z_Calloc(numtextures * sizeof(unsigned short), PU_STATIC, NULL);
	palettetranslation = Z_Calloc(numtextures * sizeof(unsigned short), PU_STATIC, NULL);
	texturewidth = Z_Calloc(numtextures * sizeof(unsigned short), PU_STATIC, NULL);
	textureheight = Z_Calloc(numtextures * sizeof(unsigned short), PU_STATIC, NULL);

	for (i = 0; i < numtextures; i++) {
		unsigned char* png;
		int w;
		int h;

		// allocate at least one slot for each texture pointer
		textureptr[i] = (unsigned int*)Z_Malloc(1 * sizeof(unsigned int), PU_STATIC, 0);

		// get starting index for switch textures
		if (!w3sstrncasecmp(lumpinfo[t_start + i].name, "SWX", 3) && swx_start == -1) {
			swx_start = i;
		}

		texturetranslation[i] = i;
		palettetranslation[i] = 0;

		// read PNG and setup global width and heights
		png = I_PNGReadData(t_start + i, true, true, false, &w, &h, NULL, 0);

		textureptr[i][0] = 0;
		texturewidth[i] = w;
		textureheight[i] = h;

		Z_Free(png);
	}

	CON_DPrintf("%i world textures initialized\n", numtextures);
}

//
// GL_BindWorldTexture
//

void GL_BindWorldTexture(int texnum, int* width, int* height) {
	unsigned char* png;
	int w;
	int h;

	// get translation index
	texnum = texturetranslation[texnum];

	if (width) {
		*width = texturewidth[texnum];
	}
	if (height) {
		*height = textureheight[texnum];
	}

	if (curtexture == texnum) {
		return;
	}

	curtexture = texnum;

	// if texture is already in video ram
	if (textureptr[texnum][palettetranslation[texnum]]) {
		glBindTexture(GL_TEXTURE_2D, textureptr[texnum][palettetranslation[texnum]]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		if (devparm) {
			glBindCalls++;
		}
		return;
	}

	// create a new texture
	png = I_PNGReadData(t_start + texnum, false, true, true,
		&w, &h, NULL, palettetranslation[texnum]);
	
	//GL_LoadShader("GLSL/D64EXPLUS.vert", "GLSL/D64EXPLUS.frag"); //Works however when it plays it crashes

	glGenTextures(1, &textureptr[texnum][palettetranslation[texnum]]);
	glBindTexture(GL_TEXTURE_2D, textureptr[texnum][palettetranslation[texnum]]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, png);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	GL_CheckFillMode();
	GL_SetTextureFilter();

	// update global width and heights
	texturewidth[texnum] = w;
	textureheight[texnum] = h;

	if (width) {
		*width = texturewidth[texnum];
	}
	if (height) {
		*height = textureheight[texnum];
	}

	Z_Free(png);

	if (devparm) {
		glBindCalls++;
	}
}

//
// GL_SetNewPalette
//
void GL_SetNewPalette(int id, unsigned char palID) {
	palettetranslation[id] = palID;

}

//
// SetTextureImage
//

static void SetTextureImage(unsigned char* data, int bits, int* origwidth, int* origheight, int format, int type) {
	glTexImage2D(
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
	int i = 0;

	g_start = W_GetNumForName("SYMBOLS");
	g_end = W_GetNumForName("MOUNTC");
	numgfx = (g_end - g_start) + 1;
	gfxptr = Z_Calloc(numgfx * sizeof(unsigned int), PU_STATIC, NULL);
	gfxwidth = Z_Calloc(numgfx * sizeof(short), PU_STATIC, NULL);
	gfxorigwidth = Z_Calloc(numgfx * sizeof(short), PU_STATIC, NULL);
	gfxheight = Z_Calloc(numgfx * sizeof(short), PU_STATIC, NULL);
	gfxorigheight = Z_Calloc(numgfx * sizeof(short), PU_STATIC, NULL);

	for (i = 0; i < numgfx; i++) {
		unsigned char* png;
		int w;
		int h;

		png = I_PNGReadData(g_start + i, true, true, false, &w, &h, NULL, 0);

		gfxptr[i] = 0;
		gfxwidth[i] = w;
		gfxorigwidth[i] = w;
		gfxorigheight[i] = h;
		gfxheight[i] = h;

		Z_Free(png);
	}

	CON_DPrintf("%i generic textures initialized\n", numgfx);
}

//
// GL_BindGfxTexture
//

int GL_BindGfxTexture(const char* name, boolean alpha) {
	unsigned char* png;
	int lump;
	int width;
	int height;
	int format;
	int type;
	int gfxid;

	lump = W_GetNumForName(name);
	gfxid = (lump - g_start);

	if (gfxid == curgfx) {
		return gfxid;
	}

	curgfx = gfxid;

	// if texture is already in video ram
	if (gfxptr[gfxid]) {
		glBindTexture(GL_TEXTURE_2D, gfxptr[gfxid]);
		if (devparm) {
			glBindCalls++;
		}
		return gfxid;
	}

	png = I_PNGReadData(lump, false, true, alpha, &width, &height, NULL, 0);

	glGenTextures(1, &gfxptr[gfxid]);
	glBindTexture(GL_TEXTURE_2D, gfxptr[gfxid]);

	// if alpha is specified, setup the format for only RGBA pixels (4 bytes) per pixel
	format = alpha ? GL_RGBA8 : GL_RGB8;
	type = alpha ? GL_RGBA : GL_RGB;

	SetTextureImage(png, (alpha ? 4 : 3), &width, &height, format, type);
	Z_Free(png);

	gfxwidth[gfxid] = width;
	gfxheight[gfxid] = height;

	if (devparm) {
		glBindCalls++;
	}

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
	spritewidth = (unsigned short*)Z_Malloc(numsprtex * sizeof(unsigned short), PU_STATIC, 0);
	spriteoffset = (float*)Z_Malloc(numsprtex * sizeof(float), PU_STATIC, 0);
	spritetopoffset = (float*)Z_Malloc(numsprtex * sizeof(float), PU_STATIC, 0);
	spriteheight = (unsigned short*)Z_Malloc(numsprtex * sizeof(unsigned short), PU_STATIC, 0);
	spriteptr = (unsigned int**)Z_Malloc(sizeof(unsigned int*) * numsprtex, PU_STATIC, 0);
	spritecount = (unsigned short*)Z_Calloc(numsprtex * sizeof(unsigned short), PU_STATIC, 0);

	// gather # of sprites per texture pointer
	for (i = 0; i < numsprtex; i++) {
		spritecount[i]++;

		for (j = 0; j < NUMSPRITES; j++) {
			// start looking for external palette lumps
			if (!strncmp(lumpinfo[s_start + i].name, sprnames[j], 4)) {
				int8_t palname[9];

				// increase the count if a palette lump is found
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
		unsigned char* png;
		int w;
		int h;
		unsigned int x;

		// allocate # of sprites per pointer
		spriteptr[i] = (unsigned int*)Z_Malloc(spritecount[i] * sizeof(unsigned int), PU_STATIC, 0);

		// reset references
		for (x = 0; x < spritecount[i]; x++) {
			spriteptr[i][x] = 0;
		}

		// read data and setup globals
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
	unsigned char* png;
	int w;
	int h;

	if ((spritenum == cursprite) && (pal == curtrans)) {
		return;
	}

	// switch to default palette if pal is invalid
	if (pal && pal >= spritecount[spritenum]) {
		pal = 0;
	}

	cursprite = spritenum;
	curtrans = pal;

	// if texture is already in video ram
	if (spriteptr[spritenum][pal]) {
		glBindTexture(GL_TEXTURE_2D, spriteptr[spritenum][pal]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		if (devparm) {
			glBindCalls++;
		}
		return;
	}

	png = I_PNGReadData(s_start + spritenum, false, true, true, &w, &h, NULL, pal);

	glGenTextures(1, &spriteptr[spritenum][pal]);
	glBindTexture(GL_TEXTURE_2D, spriteptr[spritenum][pal]);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	SetTextureImage(png, 4, &w, &h, GL_RGBA8, GL_RGBA);
	Z_Free(png);

	spritewidth[spritenum] = w;
	spriteheight[spritenum] = h;

	if (devparm) {
		glBindCalls++;
	}
}

//
// GL_ScreenToTexture
//

unsigned int GL_ScreenToTexture(void) {
	unsigned int id;
	int width;
	int height;

	glEnable(GL_TEXTURE_2D);

	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	width = GL_PadTextureDims(video_width);
	height = GL_PadTextureDims(video_height);

	glTexImage2D(
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

	glCopyTexSubImage2D(
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

static unsigned int dummytexture = 0;

void GL_BindDummyTexture(void) {
	if (dummytexture == 0) {
		//
		// build dummy texture
		//

		unsigned char rgb[48];   // 4x4 RGB texture

		memset(rgb, 0xff, 48);

		glGenTextures(1, &dummytexture);
		glBindTexture(GL_TEXTURE_2D, dummytexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, 4, 4, 0, GL_RGB, GL_UNSIGNED_BYTE, rgb);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		GL_CheckFillMode();
		GL_SetTextureFilter();
	}
	else {
		glBindTexture(GL_TEXTURE_2D, dummytexture);
	}
}

//
// GL_BindEnvTexture
//

static unsigned int envtexture = 0;

void GL_BindEnvTexture(void) {
	unsigned int rgb[16];

	memset(rgb, 0xff, sizeof(unsigned int) * 16);

	if (envtexture == 0) {
		glGenTextures(1, &envtexture);
		glBindTexture(GL_TEXTURE_2D, envtexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, (unsigned char*)rgb);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		GL_CheckFillMode();
		GL_SetTextureFilter();
	}
	else {
		glBindTexture(GL_TEXTURE_2D, envtexture);
	}
}

//
// GL_UpdateEnvTexture
//

static unsigned int lastenvcolor = 0;

void GL_UpdateEnvTexture(unsigned int color) {
	unsigned int env;
	unsigned int rgb[16];
	unsigned char* c;
	int i;

	if (lastenvcolor == color) {
		return;
	}

	glActiveTexture(GL_TEXTURE1_ARB);

	env = color;
	lastenvcolor = color;
	c = (unsigned char*)rgb;

	memset(rgb, 0, sizeof(unsigned int) * 16);

	for (i = 0; i < 16; i++) {
		*c++ = (unsigned char)((env >> 0) & 0xff);
		*c++ = (unsigned char)((env >> 8) & 0xff);
		*c++ = (unsigned char)((env >> 16) & 0xff);
		*c++ = (unsigned char)((env >> 24) & 0xff);
	}

	glTexSubImage2D(
		GL_TEXTURE_2D,
		0,
		0,
		0,
		4,
		4,
		GL_RGBA,
		GL_UNSIGNED_BYTE,
		(unsigned char*)rgb
	);

	glActiveTexture(GL_TEXTURE0_ARB);
}

//
// GL_UnloadTexture
//

void GL_UnloadTexture(unsigned int* texture) {
	if (*texture != 0) {
		glDeleteTextures(1, texture);
		*texture = 0;
	}
}

//
// GL_SetTextureUnit
//

void GL_SetTextureUnit(int unit, boolean enable) {

	if (unit > 3) {
		return;
	}

	if (curunit == unit) {
		return;
	}

	curunit = unit;

	glActiveTexture(GL_TEXTURE0_ARB + unit);
	GL_SetState(GLSTATE_TEXTURE0 + unit, enable);
}

//
// GL_SetTextureMode
//

void GL_SetTextureMode(int mode) {
	gl_env_state_t* state;

	state = &gl_env_state[curunit];

	if (state->mode == mode) {
		return;
	}

	state->mode = mode;
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, state->mode);
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

	glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, f);
}

//
// GL_InitTextures
//

void GL_InitTextures(void) {
	CON_DPrintf("--------Initializing textures--------\n");

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
