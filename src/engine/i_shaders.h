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
//      Shaders
//
//-----------------------------------------------------------------------------

#ifndef I_SHADERS_H
#define I_SHADERS_H

#include "tables.h"
#include "m_fixed.h"
#include "p_mobj.h"
#include "doomtype.h"
#include "gl_main.h"

void I_ShaderBind(void);
void I_ShaderUnBind(void);
void I_ShaderSetTextureSize(int w, int h);
void I_ShaderSetUseTexture(int on);
void I_OverlayTintShaderInit(void);
void I_ShaderFullscreenTint(float r, float g, float b, float a);

typedef char GLchar;
typedef int GLint;
typedef int GLsizei;
typedef unsigned int GLuint;
typedef unsigned int GLenum;

#ifndef GL_VERTEX_SHADER
#define GL_VERTEX_SHADER    0x8B31
#define GL_FRAGMENT_SHADER  0x8B30
#define GL_COMPILE_STATUS   0x8B81
#define GL_LINK_STATUS      0x8B82
#define GL_INFO_LOG_LENGTH  0x8B84
#endif

#endif
