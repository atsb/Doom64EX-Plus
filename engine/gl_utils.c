// Emacs style mode select   -*- C -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 2007-2012 Samuel Villarreal
// Copyright(C) 2022-2023 André Guilherme
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
// DESCRIPTION:
//		It stores some OpenGL utils here:
// 
//-----------------------------------------------------------------------------
//
// DESCRIPTION: Inlined OpenGL-exclusive functions
//
//-----------------------------------------------------------------------------


#ifdef __APPLE__
#include <math.h>
#endif

#include "doomdef.h"
#include "doomstat.h"
#include "gl_main.h"
#include "gl_texture.h"
#include "con_console.h"
#include "i_system.h"
#include "gl_utils.h"
#ifdef NIGHTDIVE
#include "i_video.h"
rbState_t rbState;
#endif
#define MAXINDICES  0x10000

unsigned short statindice = 0;

static unsigned short indicecnt = 0;
static unsigned short drawIndices[MAXINDICES];

void glGetVersion(int major, int minor)
{
	OGL_WINDOW_HINT(OGL_MAJOR_VERSION, major);
	OGL_WINDOW_HINT(OGL_MINOR_VERSION, minor);
}

void glDestroyWindow(OGL_DEFS)
{
	if (Window)
	{
		OGL_DESTROY_WINDOW(Window);
		Window = NULL;
	}
}

//
// glLogError
//

#ifdef USE_DEBUG_GLFUNCS
void glLogError(const char* message, const char* file, int line) {
	GLint err = glGetError();
	if (err != GL_NO_ERROR) {
		char str[64];

		switch (err) {
		case GL_INVALID_ENUM:
			strcpy(str, "INVALID_ENUM");
			break;
		case GL_INVALID_VALUE:
			strcpy(str, "INVALID_VALUE");
			break;
		case GL_INVALID_OPERATION:
			strcpy(str, "INVALID_OPERATION");
			break;
		case GL_STACK_OVERFLOW:
			strcpy(str, "STACK_OVERFLOW");
			break;
		case GL_STACK_UNDERFLOW:
			strcpy(str, "STACK_UNDERFLOW");
			break;
		case GL_OUT_OF_MEMORY:
			strcpy(str, "OUT_OF_MEMORY");
			break;
		default:
			sprintf(str, "0x%x", err);
			break;
		}

		I_Printf("\nGL ERROR (%s) on gl function: %s (file = %s, line = %i)\n\n", str, message, file, line);
		I_Sleep(1);
	}
}
#endif

//
// glSetVertex
//

static vtx_t* gl_prevptr = NULL;

void glSetVertex(vtx_t* vtx) {
#ifdef LOG_GLFUNC_CALLS
	I_Printf("glSetVertex(vtx=0x%p)\n", vtx);
#endif

	// 20120623 villsa - avoid redundant calls by checking for
	// the previous pointer that was set
	if (gl_prevptr == vtx) {
		return;
	}

	glTexCoordPointer(2, GL_FLOAT, sizeof(vtx_t), &vtx->tu);
	glVertexPointer(3, GL_FLOAT, sizeof(vtx_t), vtx);
	glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(vtx_t), &vtx->r);

	gl_prevptr = vtx;
}

//
// glTriangle
//

void glTriangle(int v0, int v1, int v2) {
#ifdef LOG_GLFUNC_CALLS
	I_Printf("glTriangle(v0=%i, v1=%i, v2=%i)\n", v0, v1, v2);
#endif
	if (indicecnt + 3 >= MAXINDICES) {
		I_Error("Triangle indice overflow");
	}

	drawIndices[indicecnt++] = v0;
	drawIndices[indicecnt++] = v1;
	drawIndices[indicecnt++] = v2;
}

//
// glDrawGeometry
//

void glDrawGeometry(int count, vtx_t* vtx) {
#ifdef LOG_GLFUNC_CALLS
	I_Printf("glDrawGeometry(count=0x%x, vtx=0x%p)\n", count, vtx);
#endif

	glDrawElements(GL_TRIANGLES, indicecnt, GL_UNSIGNED_SHORT, drawIndices);


	if (devparm) {
		statindice += indicecnt;
	}

	indicecnt = 0;
}

//
// glViewFrustum
//

void glViewFrustum(int width, int height, float fovy, float znear) {
	float left;
	float right;
	float bottom;
	float top;
	float aspect;
	float m[16];

#ifdef LOG_GLFUNC_CALLS
	I_Printf("glViewFrustum(width=%i, height=%i, fovy=%f, znear=%f)\n", width, height, fovy, znear);
#endif

	aspect = (float)width / (float)height;
	top = znear * (float)tan((double)fovy * M_PI / 360.0f);
	bottom = -top;
	left = bottom * aspect;
	right = top * aspect;

	m[0] = (2 * znear) / (right - left);
	m[4] = 0;
	m[8] = (right + left) / (right - left);
	m[12] = 0;

	m[1] = 0;
	m[5] = (2 * znear) / (top - bottom);
	m[9] = (top + bottom) / (top - bottom);
	m[13] = 0;

	m[2] = 0;
	m[6] = 0;
	m[10] = -1;
	m[14] = -2 * znear;

	m[3] = 0;
	m[7] = 0;
	m[11] = -1;
	m[15] = 0;

	glMultMatrixf(m);
}

//
// glSetVertexColor
//

void glSetVertexColor(vtx_t* v, unsigned int color, unsigned short count) {
	int i = 0;
#ifdef LOG_GLFUNC_CALLS
	I_Printf("glSetVertexColor(v=0x%p, c=0x%x, count=0x%x)\n", v, c, count);
#endif
	for (i = 0; i < count; i++) {
		*(unsigned int*)&v[i].r = color;
	}
}

//
// glGetColorf
//

void glGetColorf(unsigned int color, float* argb) {
#ifdef LOG_GLFUNC_CALLS
	I_Printf("glGetColorf(color=0x%x, argb=0x%p)\n", color, argb);
#endif
	argb[3] = (float)((color >> 24) & 0xff) / 255.0f;
	argb[2] = (float)((color >> 16) & 0xff) / 255.0f;
	argb[1] = (float)((color >> 8) & 0xff) / 255.0f;
	argb[0] = (float)(color & 0xff) / 255.0f;
}

#ifdef NIGHTDIVE
//
// RB_RoundPowerOfTwo
//

int RB_RoundPowerOfTwo(int x)
{
    int mask = 1;

    while (mask < 0x40000000)
    {
        if (x == mask || (x & (mask - 1)) == x)
        {
            return mask;
        }

        mask <<= 1;
    }

    return x;
}

//
// RB_SetTexParameters
//

void RB_SetTexParameters(rbTexture_t* rbTexture)
{
    unsigned int clamp;
    unsigned int filter;

    switch (rbTexture->clampMode)
    {
    case TC_CLAMP:
        clamp = GL_CLAMP_TO_EDGE;
        break;

    case TC_CLAMP_BORDER:
        clamp = GL_CLAMP_TO_BORDER;
        break;

    case TC_REPEAT:
        clamp = GL_REPEAT;
        break;

    case TC_MIRRORED:
        clamp = GL_MIRRORED_REPEAT;
        break;

    default:
        return;
    }

    switch (rbTexture->filterMode)
    {
    case TF_LINEAR:
        filter = GL_LINEAR;
        break;

    case TF_NEAREST:
        filter = GL_NEAREST;
        break;

    default:
        return;
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, clamp);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, clamp);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
}

//
// RB_ChangeTexParameters
//

void RB_ChangeTexParameters(rbTexture_t* rbTexture, const texClampMode_t clamp, const texFilterMode_t filter)
{
    if (rbTexture->clampMode == clamp && rbTexture->filterMode == filter)
    {
        return;
    }

    rbTexture->clampMode = clamp;
    rbTexture->filterMode = filter;

    RB_SetTexParameters(rbTexture);
}

//
// RB_BindTexture
//

void RB_BindTexture(rbTexture_t* rbTexture)
{
    int tid;
    int currentTexture;
    int unit;

    if (!rbTexture)
    {
        return;
    }

    tid = rbTexture->texid;
    unit = rbState.currentUnit;
    currentTexture = rbState.textureUnits[unit].currentTexture;

    if (tid == currentTexture)
    {
        return;
    }

    glBindTexture(GL_TEXTURE_2D, tid);

    rbState.textureUnits[unit].currentTexture = tid;
    rbState.numTextureBinds++;
}

//
// RB_UnbindTexture
//

void RB_UnbindTexture(void)
{
    int unit = rbState.currentUnit;
    rbState.textureUnits[unit].currentTexture = 0;

    glBindTexture(GL_TEXTURE_2D, 0);
}

//
// RB_DeleteTexture
//

void RB_DeleteTexture(rbTexture_t* rbTexture)
{
    if (!rbTexture || rbTexture->texid == 0)
    {
        return;
    }

    glDeleteTextures(1, &rbTexture->texid);
    rbTexture->texid = 0;
}

//
// RB_UploadTexture
//

void RB_UploadTexture(rbTexture_t* rbTexture, byte* data, texClampMode_t clamp, texFilterMode_t filter)
{
    rbTexture->clampMode = clamp;
    rbTexture->filterMode = filter;

    glGenTextures(1, &rbTexture->texid);

    if (rbTexture->texid == 0)
    {
        // renderer is not initialized yet
        return;
    }

    glBindTexture(GL_TEXTURE_2D, rbTexture->texid);

    if (data)
    {
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            (rbTexture->colorMode == TCR_RGBA) ? GL_RGBA8 : GL_RGB8,
            rbTexture->width,
            rbTexture->height,
            0,
            (rbTexture->colorMode == TCR_RGBA) ? GL_RGBA : GL_RGB,
            GL_UNSIGNED_BYTE,
            data);
    }

    glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_FALSE);

    RB_SetTexParameters(rbTexture);
    glBindTexture(GL_TEXTURE_2D, 0);
}

//
// RB_SetReadBuffer
//

void RB_SetReadBuffer(const GLenum state)
{
    if (rbState.readBuffer == state)
    {
        return; // already set
    }

    glReadBuffer(state);
    rbState.readBuffer = state;
}

//
// RB_UpdateTexture
//

void RB_UpdateTexture(rbTexture_t* rbTexture, byte* data)
{
    if (!rbTexture || rbTexture->texid == 0)
    {
        return;
    }

    if (rbState.textureUnits[rbState.currentUnit].currentTexture != rbTexture->texid)
    {
        return;
    }

    glTexSubImage2D(
        GL_TEXTURE_2D,
        0,
        0,
        0,
        rbTexture->width,
        rbTexture->height,
        (rbTexture->colorMode == TCR_RGBA) ? GL_RGBA : GL_RGB,
        GL_UNSIGNED_BYTE,
        data);
}

//
// RB_BindFrameBuffer
//

void RB_BindFrameBuffer(rbTexture_t* rbTexture)
{
    int unit;
    int currentTexture;

    if (rbTexture->texid == 0)
    {
        glGenTextures(1, &rbTexture->texid);
    }

    unit = rbState.currentUnit;
    currentTexture = rbState.textureUnits[unit].currentTexture;

    if (rbTexture->texid != currentTexture)
    {
        glBindTexture(GL_TEXTURE_2D, rbTexture->texid);
        rbState.textureUnits[unit].currentTexture = rbTexture->texid;
    }

    RB_SetReadBuffer(GL_BACK);

#if 0
    rbTexture->origwidth = SDL_GetWindowSurface(windowscreen)->w;
    rbTexture->origheight = SDL_GetWindowSurface(windowscreen)->h;
#else
    int w;
    int h;
    SDL_GetWindowSize(window, &w, &h);
    rbTexture->origwidth = w;
    rbTexture->origheight = h;
#endif
    rbTexture->width = rbTexture->origwidth;
    rbTexture->height = rbTexture->origheight;

    glCopyTexImage2D(GL_TEXTURE_2D,
        0,
        GL_RGBA8,
        0,
        0,
        rbTexture->origwidth,
        rbTexture->origheight,
        0);
    RB_SetTexParameters(rbTexture);
}

//
// RB_BindDepthBuffer
//

void RB_BindDepthBuffer(rbTexture_t* rbTexture)
{
    int unit;
    int currentTexture;

    if (rbTexture->texid == 0)
    {
        glGenTextures(1, &rbTexture->texid);
    }

    unit = rbState.currentUnit;
    currentTexture = rbState.textureUnits[unit].currentTexture;

    if (rbTexture->texid != currentTexture)
    {
        glBindTexture(GL_TEXTURE_2D, rbTexture->texid);
        rbState.textureUnits[unit].currentTexture = rbTexture->texid;
    }

#if 1
    int w;
    int h;
    SDL_GetWindowSize(window, &w, &h);
    rbTexture->origwidth = w;
    rbTexture->origheight = h;
#else
    rbTexture->origwidth = SDL_GetWindowSurface(windowscreen)->w;
    rbTexture->origheight = SDL_GetWindowSurface(windowscreen)->h;
#endif
    rbTexture->width = rbTexture->origwidth;
    rbTexture->height = rbTexture->origheight;

    glCopyTexImage2D(GL_TEXTURE_2D,
        0,
        GL_DEPTH_COMPONENT,
        0,
        0,
        rbTexture->origwidth,
        rbTexture->origheight,
        0);

    RB_SetTexParameters(rbTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
}

//
// RB_ClearBuffer
//

void RB_ClearBuffer(const glClearBit_t bit)
{
    int clearBit = 0;

    if (bit == GLCB_ALL)
    {
        clearBit = (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    }
    else
    {
        if (bit & GLCB_COLOR)
        {
            clearBit |= GL_COLOR_BUFFER_BIT;
        }
        if (bit & GLCB_DEPTH)
        {
            clearBit |= GL_DEPTH_BUFFER_BIT;
        }
        if (bit & GLCB_STENCIL)
        {
            clearBit |= GL_STENCIL_BUFFER_BIT;
        }
    }

    glClear(clearBit);
}

//
// RB_SetCull
//

void RB_SetCull(int type)
{
    int pCullType = rbState.cullType ^ type;
    int cullType = 0;

    if (pCullType == 0)
        return; // already set

    switch (type)
    {
    case GLCULL_FRONT:
        cullType = GL_FRONT;
        break;

    case GLCULL_BACK:
        cullType = GL_BACK;
        break;

    default:
        return;
    }

    glCullFace(cullType);
    rbState.cullType = type;
    rbState.numStateChanges++;
}
#define GL_TEXTURE0_ARB				0x84C0
#define GL_TEXTURE1_ARB				0x84C1
void RB_SetTextureUnit(int unit)
{
    if (unit > MAX_TEXTURE_UNITS || unit < 0)
    {
        return;
    }

    if (unit == rbState.currentUnit)
    {
        return; // already binded
    }

#ifndef SVE_PLAT_SWITCH
    glActiveTexture(GL_TEXTURE0_ARB + unit);
    glClientActiveTexture(GL_TEXTURE0_ARB + unit);
#endif
    rbState.currentUnit = unit;
}

//
// RB_SetState
//

void RB_SetState(const int bits, boolean bEnable)
{
    int stateFlag = 0;

    switch (bits)
    {
    case DGLSTATE_BLEND:
        stateFlag = GL_BLEND;
        break;

    case DGLSTATE_CULL:
        stateFlag = GL_CULL_FACE;
        break;

    case DGLSTATE_TEXTURE0:
        RB_SetTextureUnit(0);
        stateFlag = GL_TEXTURE_2D;
        break;

    case DGLSTATE_TEXTURE1:
        RB_SetTextureUnit(1);
        stateFlag = GL_TEXTURE_2D;
        break;

    case DGLSTATE_TEXTURE2:
        RB_SetTextureUnit(2);
        stateFlag = GL_TEXTURE_2D;
        break;

    case DGLSTATE_TEXTURE3:
        RB_SetTextureUnit(3);
        stateFlag = GL_TEXTURE_2D;
        break;

    case GLSTATE_ALPHATEST:
        stateFlag = GL_ALPHA_TEST;
        break;

    case GLSTATE_TEXGEN_S:
        stateFlag = GL_TEXTURE_GEN_S;
        break;

    case GLSTATE_TEXGEN_T:
        stateFlag = GL_TEXTURE_GEN_T;
        break;

    case GLSTATE_DEPTHTEST:
        stateFlag = GL_DEPTH_TEST;
        break;

    case GLSTATE_FOG:
        stateFlag = GL_FOG;
        break;

    case GLSTATE_STENCILTEST:
        stateFlag = GL_STENCIL_TEST;
        break;

    case GLSTATE_SCISSOR:
        stateFlag = GL_SCISSOR_TEST;
        break;

    default:
        return;
    }

    // if state was already set then don't set it again
    if (bEnable && !(rbState.glStateBits & (1 << bits)))
    {
        glEnable(stateFlag);
        rbState.glStateBits |= (1 << bits);
        rbState.numStateChanges++;
    }
    // if state was already unset then don't unset it again
    else if (!bEnable && (rbState.glStateBits & (1 << bits)))
    {
        glDisable(stateFlag);
        rbState.glStateBits &= ~(1 << bits);
        rbState.numStateChanges++;
    }
}


//
// RB_DrawScreenTexture
//
// Draws a full screen texture that fits within the
// texture's aspect ratio
//

void RB_DrawScreenTexture(rbTexture_t* texture, const int width, const int height)
{
    vtx_t v[4];
    float tx;
    int i;
    SDL_Surface* screen;
    int wi, hi, ws, hs;
    float ri, rs;
    int scalew, scaleh;
    int xoffs = 0, yoffs = 0;

    RB_BindTexture(texture);

    screen = SDL_GetWindowSurface(window);
    ws = screen->w;
    hs = screen->h;

    tx = (float)texture->origwidth / (float)texture->width;

    for (i = 0; i < 4; ++i)
    {
        v[i].r = v[i].g = v[i].b = v[i].a = 0xff;
        v[i].z = 0;
    }

    wi = width;
    hi = height;

    rs = (float)ws / hs;
    ri = (float)wi / hi;

    if (rs > ri)
    {
        scalew = wi * hs / hi;
        scaleh = hs;
    }
    else
    {
        scalew = ws;
        scaleh = hi * ws / wi;
    }

    if (scalew < ws)
    {
        xoffs = (ws - scalew) / 2;
    }
    if (scaleh < hs)
    {
        yoffs = (hs - scaleh) / 2;
    }

    v[0].x = v[2].x = xoffs;
    v[0].y = v[1].y = yoffs;
    v[1].x = v[3].x = xoffs + scalew;
    v[2].y = v[3].y = yoffs + scaleh;

    v[0].tu = v[2].tu = 0;
    v[0].tv = v[1].tv = 0;
    v[1].tu = v[3].tu = tx;
    v[2].tv = v[3].tv = 1;

    RB_SetState(GLSTATE_CULL, true);
    RB_SetCull(GLCULL_FRONT);
    RB_SetState(GLSTATE_DEPTHTEST, false);

    // render
    glBegin(GL_QUADS);
    glColor4ubv(&v[0].r);
    glTexCoord2fv(&v[0].tu); glVertex3fv(&v[0].x);
    glTexCoord2fv(&v[1].tu); glVertex3fv(&v[1].x);
    glTexCoord2fv(&v[3].tu); glVertex3fv(&v[3].x);
    glTexCoord2fv(&v[2].tu); glVertex3fv(&v[2].x);
    glEnd();
}

//
// MTX_Copy
//

void MTX_Copy(matrix dst, matrix src)
{
    memcpy(dst, src, sizeof(matrix));
}

//
// MTX_IsUninitialized
//

boolean MTX_IsUninitialized(matrix m)
{
    int i;
    float f = 0;

    for (i = 0; i < 16; i++)
    {
        f += m[i];
    }

    return (f == 0);
}

//
// MTX_Identity
//

void MTX_Identity(matrix m)
{
    m[0] = 1;
    m[1] = 0;
    m[2] = 0;
    m[3] = 0;
    m[4] = 0;
    m[5] = 1;
    m[6] = 0;
    m[7] = 0;
    m[8] = 0;
    m[9] = 0;
    m[10] = 1;
    m[11] = 0;
    m[12] = 0;
    m[13] = 0;
    m[14] = 0;
    m[15] = 1;
}

//
// MTX_SetTranslation
//

void MTX_SetTranslation(matrix m, float x, float y, float z)
{
    m[12] = x;
    m[13] = y;
    m[14] = z;
}

//
// MTX_Scale
//

void MTX_Scale(matrix m, float x, float y, float z)
{
    m[0] = x * m[0];
    m[1] = x * m[1];
    m[2] = x * m[2];
    m[4] = y * m[4];
    m[5] = y * m[5];
    m[6] = y * m[6];
    m[8] = z * m[8];
    m[9] = z * m[9];
    m[10] = z * m[10];
}

//
// MTX_IdentityX
//

void MTX_IdentityX(matrix m, float angle)
{
    float s = sinf(angle);
    float c = cosf(angle);

    m[0] = 1;
    m[1] = 0;
    m[2] = 0;
    m[3] = 0;
    m[4] = 0;
    m[5] = c;
    m[6] = s;
    m[7] = 0;
    m[8] = 0;
    m[9] = -s;
    m[10] = c;
    m[11] = 0;
    m[12] = 0;
    m[13] = 0;
    m[14] = 0;
    m[15] = 1;
}

//
// MTX_IdentityY
//

void MTX_IdentityY(matrix m, float angle)
{
    float s = sinf(angle);
    float c = cosf(angle);

    m[0] = c;
    m[1] = 0;
    m[2] = -s;
    m[3] = 0;
    m[4] = 0;
    m[5] = 1;
    m[6] = 0;
    m[7] = 0;
    m[8] = s;
    m[9] = 0;
    m[10] = c;
    m[11] = 0;
    m[12] = 0;
    m[13] = 0;
    m[14] = 0;
    m[15] = 1;
}

//
// MTX_IdentityZ
//

void MTX_IdentityZ(matrix m, float angle)
{
    float s = sinf(angle);
    float c = cosf(angle);

    m[0] = c;
    m[1] = s;
    m[2] = 0;
    m[3] = 0;
    m[4] = -s;
    m[5] = c;
    m[6] = 0;
    m[7] = 0;
    m[8] = 0;
    m[9] = 0;
    m[10] = 1;
    m[11] = 0;
    m[12] = 0;
    m[13] = 0;
    m[14] = 0;
    m[15] = 1;
}

//
// MTX_IdentityTranspose
//

void MTX_IdentityTranspose(matrix dst, matrix src)
{
    int i;

    for (i = 0; i < 4; ++i)
    {
        dst[0 * 4 + i] = src[0 * 4 + i];
    }

    for (i = 0; i < 4; ++i)
    {
        dst[1 * 4 + i] = src[2 * 4 + i];
    }

    for (i = 0; i < 4; ++i)
    {
        dst[2 * 4 + i] = src[1 * 4 + i];
    }

    for (i = 0; i < 4; ++i)
    {
        dst[3 * 4 + i] = src[3 * 4 + i];
    }
}

//
// MTX_RotateX
//

void MTX_RotateX(matrix m, float angle)
{
    float s;
    float c;
    float tm1;
    float tm5;
    float tm9;
    float tm13;

    s = sinf(angle);
    c = cosf(angle);

    tm1 = m[1];
    tm5 = m[5];
    tm9 = m[9];
    tm13 = m[13];

    m[1] = tm1 * c - s * m[2];
    m[2] = c * m[2] + tm1 * s;
    m[5] = tm5 * c - s * m[6];
    m[6] = c * m[6] + tm5 * s;
    m[9] = tm9 * c - s * m[10];
    m[10] = c * m[10] + tm9 * s;
    m[13] = tm13 * c - s * m[14];
    m[14] = c * m[14] + tm13 * s;
}

//
// MTX_RotateY
//

void MTX_RotateY(matrix m, float angle)
{
    float s;
    float c;
    float tm0;
    float tm1;
    float tm2;

    s = sinf(angle);
    c = cosf(angle);

    tm0 = m[0];
    tm1 = m[1];
    tm2 = m[2];

    m[0] = tm0 * c - s * m[8];
    m[8] = c * m[8] + tm0 * s;
    m[1] = tm1 * c - s * m[9];
    m[9] = c * m[9] + tm1 * s;
    m[2] = tm2 * c - s * m[10];
    m[10] = c * m[10] + tm2 * s;
}

//
// MTX_RotateZ
//

void MTX_RotateZ(matrix m, float angle)
{
    float s;
    float c;
    float tm0;
    float tm8;
    float tm4;
    float tm12;

    s = sinf(angle);
    c = cosf(angle);

    tm0 = m[0];
    tm4 = m[4];
    tm8 = m[8];
    tm12 = m[12];

    m[0] = s * m[2] + tm0 * c;
    m[2] = c * m[2] - tm0 * s;
    m[4] = s * m[6] + tm4 * c;
    m[6] = c * m[6] - tm4 * s;
    m[8] = s * m[10] + tm8 * c;
    m[10] = c * m[10] - tm8 * s;
    m[12] = s * m[14] + tm12 * c;
    m[14] = c * m[14] - tm12 * s;
}

//
// MTX_ViewFrustum
//

void MTX_ViewFrustum(matrix m, int width, int height, float fovy, float znear) {
    float left;
    float right;
    float bottom;
    float top;
    float aspect;

    aspect = (float)width / (float)height;
    top = znear * (float)tan((float)fovy * M_PI / 360.0f);
    bottom = -top;
    left = bottom * aspect;
    right = top * aspect;

    m[0] = (2 * znear) / (right - left);
    m[4] = 0;
    m[8] = (right + left) / (right - left);
    m[12] = 0;

    m[1] = 0;
    m[5] = (2 * znear) / (top - bottom);
    m[9] = (top + bottom) / (top - bottom);
    m[13] = 0;

    m[2] = 0;
    m[6] = 0;
    m[10] = -1;
    m[14] = -2 * znear;

    m[3] = 0;
    m[7] = 0;
    m[11] = -1;
    m[15] = 0;
}

//
// MTX_SetOrtho
//

void MTX_SetOrtho(matrix m, float left, float right, float bottom, float top, float zNear, float zFar)
{
    m[0] = 2 / (right - left);
    m[5] = 2 / (top - bottom);
    m[10] = -2 / (zFar - zNear);

    m[12] = -(right + left) / (right - left);
    m[13] = -(top + bottom) / (top - bottom);
    m[14] = -(zFar + zNear) / (zFar - zNear);
    m[15] = 1;

    m[1] = 0;
    m[2] = 0;
    m[3] = 0;
    m[4] = 0;
    m[6] = 0;
    m[7] = 0;
    m[8] = 0;
    m[9] = 0;
    m[11] = 0;
}

//
// MTX_Multiply
//

void MTX_Multiply(matrix m, matrix m1, matrix m2)
{
    m[0] = m1[1] * m2[4] + m2[8] * m1[2] + m1[3] * m2[12] + m1[0] * m2[0];
    m[1] = m1[0] * m2[1] + m2[5] * m1[1] + m2[9] * m1[2] + m2[13] * m1[3];
    m[2] = m1[0] * m2[2] + m2[10] * m1[2] + m2[14] * m1[3] + m2[6] * m1[1];
    m[3] = m1[0] * m2[3] + m2[15] * m1[3] + m2[7] * m1[1] + m2[11] * m1[2];
    m[4] = m2[0] * m1[4] + m1[7] * m2[12] + m1[5] * m2[4] + m1[6] * m2[8];
    m[5] = m1[4] * m2[1] + m1[5] * m2[5] + m1[7] * m2[13] + m1[6] * m2[9];
    m[6] = m1[5] * m2[6] + m1[7] * m2[14] + m1[4] * m2[2] + m1[6] * m2[10];
    m[7] = m1[6] * m2[11] + m1[7] * m2[15] + m1[5] * m2[7] + m1[4] * m2[3];
    m[8] = m2[0] * m1[8] + m1[10] * m2[8] + m1[11] * m2[12] + m1[9] * m2[4];
    m[9] = m1[8] * m2[1] + m1[10] * m2[9] + m1[11] * m2[13] + m1[9] * m2[5];
    m[10] = m1[9] * m2[6] + m1[10] * m2[10] + m1[11] * m2[14] + m1[8] * m2[2];
    m[11] = m1[9] * m2[7] + m1[11] * m2[15] + m1[10] * m2[11] + m1[8] * m2[3];
    m[12] = m2[0] * m1[12] + m2[12] * m1[15] + m2[4] * m1[13] + m2[8] * m1[14];
    m[13] = m2[13] * m1[15] + m2[1] * m1[12] + m2[9] * m1[14] + m2[5] * m1[13];
    m[14] = m2[6] * m1[13] + m2[14] * m1[15] + m2[10] * m1[14] + m2[2] * m1[12];
    m[15] = m2[3] * m1[12] + m2[7] * m1[13] + m2[11] * m1[14] + m2[15] * m1[15];
}

//
// MTX_MultiplyRotations
//

void MTX_MultiplyRotations(matrix m, matrix m1, matrix m2)
{
    m[0] = m2[4] * m1[1] + m1[2] * m2[8] + m2[0] * m1[0];
    m[1] = m1[0] * m2[1] + m2[9] * m1[2] + m2[5] * m1[1];
    m[2] = m1[0] * m2[2] + m1[1] * m2[6] + m1[2] * m2[10];
    m[3] = 0;
    m[4] = m2[0] * m1[4] + m2[4] * m1[5] + m1[6] * m2[8];
    m[5] = m2[5] * m1[5] + m1[6] * m2[9] + m1[4] * m2[1];
    m[6] = m1[5] * m2[6] + m1[6] * m2[10] + m1[4] * m2[2];
    m[7] = 0;
    m[8] = m2[0] * m1[8] + m1[10] * m2[8] + m1[9] * m2[4];
    m[9] = m1[8] * m2[1] + m1[9] * m2[5] + m1[10] * m2[9];
    m[10] = m1[8] * m2[2] + m1[9] * m2[6] + m1[10] * m2[10];
    m[11] = 0;
    m[12] = m2[0] * m1[12] + m1[14] * m2[8] + m1[13] * m2[4] + m2[12];
    m[13] = m1[13] * m2[5] + m1[14] * m2[9] + m1[12] * m2[1] + m2[13];
    m[14] = m1[12] * m2[2] + m1[14] * m2[10] + m1[13] * m2[6] + m2[14];
    m[15] = 1;
}

//
// MTX_MultiplyVector
//

void MTX_MultiplyVector(matrix m, float* xyz)
{
    float _x = xyz[0];
    float _y = xyz[1];
    float _z = xyz[2];

    xyz[0] = m[4] * _y + m[8] * _z + m[0] * _x + m[12];
    xyz[1] = m[5] * _y + m[9] * _z + m[1] * _x + m[13];
    xyz[2] = m[6] * _y + m[10] * _z + m[2] * _x + m[14];
}

//
// MTX_MultiplyVector4
//

void MTX_MultiplyVector4(matrix m, float* xyzw)
{
    float _x = xyzw[0];
    float _y = xyzw[1];
    float _z = xyzw[2];
    //float _w = xyzw[3];

    xyzw[0] = m[4] * _y + m[8] * _z + m[0] * _x + m[12];
    xyzw[1] = m[5] * _y + m[9] * _z + m[1] * _x + m[13];
    xyzw[2] = m[6] * _y + m[10] * _z + m[2] * _x + m[14];
    xyzw[3] = m[7] * _y + m[11] * _z + m[3] * _x + m[15];
}

//
// MTX_Invert
//

void MTX_Invert(matrix out, matrix in)
{
    float d;
    matrix m;

    MTX_Copy(m, in);

    d = m[0] * m[10] * m[5] -
        m[0] * m[9] * m[6] -
        m[1] * m[4] * m[10] +
        m[2] * m[4] * m[9] +
        m[1] * m[6] * m[8] -
        m[2] * m[5] * m[8];

    if (d != 0.0f)
    {
        matrix inv;

        d = (1.0f / d);

        inv[0] = (m[10] * m[5] - m[9] * m[6]) * d;
        inv[1] = -((m[1] * m[10] - m[2] * m[9]) * d);
        inv[2] = (m[1] * m[6] - m[2] * m[5]) * d;
        inv[3] = 0;
        inv[4] = (m[6] * m[8] - m[4] * m[10]) * d;
        inv[5] = (m[0] * m[10] - m[2] * m[8]) * d;
        inv[6] = -((m[0] * m[6] - m[2] * m[4]) * d);
        inv[7] = 0;
        inv[8] = -((m[5] * m[8] - m[4] * m[9]) * d);
        inv[9] = (m[1] * m[8] - m[0] * m[9]) * d;
        inv[10] = -((m[1] * m[4] - m[0] * m[5]) * d);
        inv[11] = 0;
        inv[12] = ((m[13] * m[10] - m[14] * m[9]) * m[4]
            + m[14] * m[5] * m[8]
            - m[13] * m[6] * m[8]
            - m[12] * m[10] * m[5]
            + m[12] * m[9] * m[6]) * d;
        inv[13] = (m[0] * m[14] * m[9]
            - m[0] * m[13] * m[10]
            - m[14] * m[1] * m[8]
            + m[13] * m[2] * m[8]
            + m[12] * m[1] * m[10]
            - m[12] * m[2] * m[9]) * d;
        inv[14] = -((m[0] * m[14] * m[5]
            - m[0] * m[13] * m[6]
            - m[14] * m[1] * m[4]
            + m[13] * m[2] * m[4]
            + m[12] * m[1] * m[6]
            - m[12] * m[2] * m[5]) * d);
        inv[15] = 1.0f;

        MTX_Copy(out, inv);
        return;
    }

    MTX_Copy(out, m);
}

static float InvSqrt(float x)
{
    unsigned int i;
    float r;
    float y;

    y = x * 0.5f;
    i = *(unsigned int*)&x;
    i = 0x5f3759df - (i >> 1);
    r = *(float*)&i;
    r = r * (1.5f - r * r * y);

    return r;
}

//
// MTX_ToQuaternion
//
// Assumes out is an array of 4 floats
//

void MTX_ToQuaternion(matrix m, float* out)
{
    float t, d;
    float mx, my, mz;
    float m21, m20, m10;

    mx = m[0];
    my = m[5];
    mz = m[10];

    m21 = (m[9] - m[6]);
    m20 = (m[8] - m[2]);
    m10 = (m[4] - m[1]);

    t = 1.0f + mx + my + mz;

    if (t > 0)
    {
        d = 0.5f / (t * InvSqrt(t));
        out[0] = m21 * d;
        out[1] = m20 * d;
        out[2] = m10 * d;
        out[3] = 0.25f / d;
    }
    else if (mx > my && mx > mz)
    {
        t = 1.0f + mx - my - mz;
        d = (t * InvSqrt(t)) * 2;
        out[0] = 0.5f / d;
        out[1] = m10 / d;
        out[2] = m20 / d;
        out[3] = m21 / d;
    }
    else if (my > mz)
    {
        t = 1.0f + my - mx - mz;
        d = (t * InvSqrt(t)) * 2;
        out[0] = m10 / d;
        out[1] = 0.5f / d;
        out[2] = m21 / d;
        out[3] = m20 / d;
    }
    else
    {
        t = 1.0f + mz - mx - my;
        d = (t * InvSqrt(t)) * 2;
        out[0] = m20 / d;
        out[1] = m21 / d;
        out[2] = 0.5f / d;
        out[3] = m10 / d;
    }

    //
    // normalize quaternion
    // TODO: figure out why InvSqrt produces inaccurate results
    // use sqrtf for now
    //
    d = sqrtf(out[0] * out[0] +
        out[1] * out[1] +
        out[2] * out[2] +
        out[3] * out[3]);

    if (d != 0.0f)
    {
        d = 1.0f / d;
        out[0] *= d;
        out[1] *= d;
        out[2] *= d;
        out[3] *= d;
    }
}


//
// RB_SetOrtho
//

void RB_SetOrtho(void)
{
    matrix mtx;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    MTX_SetOrtho(mtx, 0, (float)SCREENWIDTH, (float)SCREENHEIGHT, 0, -1, 1);
    glLoadMatrixf(mtx);
}

//
// RB_SetMaxOrtho
//

void RB_SetMaxOrtho(int sw, int sh)
{
    matrix mtx;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    MTX_SetOrtho(mtx, 0, (float)sw, (float)sh, 0, -1, 1);
    glLoadMatrixf(mtx);
}

#endif