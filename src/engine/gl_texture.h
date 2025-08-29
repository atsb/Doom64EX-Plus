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

#ifndef __GL_TEXTURE_H__
#define __GL_TEXTURE_H__

#include "gl_main.h"

extern int                  curtexture;
extern int                  cursprite;
extern int                    curtrans;
extern int                  curgfx;

extern word* texturewidth;
extern word* textureheight;
extern dtexture** textureptr;
extern int                  t_start;
extern int                  t_end;
extern int                  swx_start;
extern int                  numtextures;
extern word* texturetranslation;
extern word* palettetranslation;

extern int                  g_start;
extern int                  g_end;
extern int                  numgfx;
extern dtexture* gfxptr;
extern word* gfxwidth;
extern word* gfxorigwidth;
extern word* gfxheight;
extern word* gfxorigheight;

extern int                  s_start;
extern int                  s_end;
extern dtexture** spriteptr;
extern int                  numsprtex;
extern word* spritewidth;
extern float* spriteoffset;
extern float* spritetopoffset;
extern word* spriteheight;

void        GL_InitTextures(void);
void        GL_UnloadTexture(dtexture* texture);
void        GL_SetTextureUnit(int unit, int enable);
void        GL_SetTextureMode(int mode);
void        GL_SetCombineState(int combine);
void        GL_SetCombineStateAlpha(int combine);
void        GL_SetEnvColor(float* param);
void        GL_SetCombineSourceRGB(int source, int target);
void        GL_SetCombineSourceAlpha(int source, int target);
void        GL_SetCombineOperandRGB(int operand, int target);
void        GL_SetCombineOperandAlpha(int operand, int target);
void        GL_BindWorldTexture(int texnum, int* width, int* height);
void        GL_BindSpriteTexture(int spritenum, int pal);
int         GL_BindGfxTexture(const char* name, int alpha);
int         GL_PadTextureDims(int size);
void        GL_SetNewPalette(int id, byte palID);
void        GL_DumpTextures(void);
void        GL_ResetTextures(void);
void        GL_BindDummyTexture(void);
void        GL_UpdateEnvTexture(rcolor color);
void        GL_BindEnvTexture(void);
dtexture    GL_ScreenToTexture(void);
int			GL_WorldTextureIsTranslucent(int texnum);
int			GL_WorldTextureIsMasked(int texnum);
void		GL_WorldTextureEnsureClassified(int texnum);
int			GL_GetGfxIdForLump(int lump);
void		GL_Env_RGB_Modulate_Alpha_FromTexture(void);
void		I_ShaderBind(void);
void		I_ShaderUnBind(void);
#endif
