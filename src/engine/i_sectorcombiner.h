// Emacs style mode select   -*- C -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 2025 Gibbon
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
//      Texture Combiner -> Shaders (Glue)
//
//-----------------------------------------------------------------------------

#ifndef I_SECTORCOMBINER_H
#define I_SECTORCOMBINER_H

// atsb
// Lightweight glue that lets the fixed function texture combiners be piped and driven by shaders.

#ifndef GL_MODULATE
#define GL_MODULATE 0x2100
#endif
#ifndef GL_ADD
#define GL_ADD 0x0104
#endif
#ifndef GL_INTERPOLATE
#define GL_INTERPOLATE 0x8575
#endif
#ifndef GL_REPLACE
#define GL_REPLACE 0x1E01
#endif
#ifndef GL_TEXTURE
#define GL_TEXTURE 0x1702
#endif
#ifndef GL_PRIMARY_COLOR
#define GL_PRIMARY_COLOR 0x8577
#endif
#ifndef GL_CONSTANT
#define GL_CONSTANT 0x8576
#endif
#ifndef GL_SRC_COLOR
#define GL_SRC_COLOR 0x0300
#endif
#ifndef GL_ONE_MINUS_SRC_COLOR
#define GL_ONE_MINUS_SRC_COLOR 0x0301
#endif

void I_SectorCombiner_Init(void);
void I_SectorCombiner_Bind(int useTexture, int texWidth, int texHeight);
void I_SectorCombiner_Unbind(void);
void I_SectorCombiner_SetEnvColor(float r, float g, float b, float a);
void I_SectorCombiner_SetCombineRGB(int mode);
void I_SectorCombiner_SetCombineAlpha(int mode);
void I_SectorCombiner_SetSourceRGB(int slot, int source);
void I_SectorCombiner_SetOperandRGB(int slot, int operand);
void I_SectorCombiner_SetFog(int enabled, float r, float g, float b, float factor);
void I_SectorCombiner_SetFogParams(int mode, float start, float end, float density);
void I_SectorCombiner_Commit(void);
int I_SectorCombiner_IsReady(void);

#endif
