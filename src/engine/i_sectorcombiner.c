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

#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include <string.h>
#include <math.h>

#include "i_sectorcombiner.h"

#ifndef COUNTOF
#define COUNTOF(a) ((int)(sizeof(a)/sizeof((a)[0])))
#endif

#define MAX_PASSES 4
#define SC_EPS 1e-5f

// texture combiner to GLSL glue...
static GLint(APIENTRY* pglGetUniformLocation)(GLuint, const GLchar*) = NULL;
static void   (APIENTRY* pglUniform1i)(GLint, int) = NULL;
static void   (APIENTRY* pglUniform1f)(GLint, float) = NULL;
static void   (APIENTRY* pglUniform2f)(GLint, float, float) = NULL;
static void   (APIENTRY* pglUniform3f)(GLint, float, float, float) = NULL;
static void   (APIENTRY* pglUniform4f)(GLint, float, float, float, float) = NULL;

static int g_procs_loaded = 0;

static void load_glsl_procs(void) {
    if (g_procs_loaded) return;
#define GL_GET(fn) *(void**)(&p##fn) = SDL_GL_GetProcAddress(#fn)
    GL_GET(glGetUniformLocation);
    GL_GET(glUniform1i);
    GL_GET(glUniform1f);
    GL_GET(glUniform2f);
    GL_GET(glUniform3f);
    GL_GET(glUniform4f);
#undef GL_GET
    g_procs_loaded = pglGetUniformLocation && pglUniform1i && pglUniform1f && pglUniform2f && pglUniform3f && pglUniform4f;
}


// atsb: this was 's' for 'shaders', I knew it would be a pain to do this so less typing is better...
static struct {
    int   combine_rgb;
    int   combine_alpha;
    int   source_rgb[3];
    int   operand_rgb[3];
    float env_color[4];

    int   pass_mode[MAX_PASSES];
    float pass_color[MAX_PASSES][4];
    float pass_factor[MAX_PASSES];
    int   pass_count;

    int   fog_enabled;
    float fog_color[3];
    float fog_factor;

    int   fog_mode;
    float fog_start;
    float fog_end;
    float fog_density;

    int   use_texture;
    int   tex_w, tex_h;
} s;

static void reset_state(void) {
    memset(&s, 0, sizeof(s));
    s.combine_rgb = GL_MODULATE;
    s.combine_alpha = GL_MODULATE;
    for (int i = 0; i < 3; i++) { 
        s.source_rgb[i] = GL_TEXTURE; 
        s.operand_rgb[i] = GL_SRC_COLOR;
    }
    s.env_color[0] = s.env_color[1] = s.env_color[2] = 0.0f; s.env_color[3] = 1.0f;
    s.use_texture = 1;
}

// atsb: we pass multiple times over, so we mimic the same way texture combiners make multiple passes
static void I_SectorCombinerTexturePasses(void) {
    s.pass_count = 0;

    float src0[3] = { 1,1,1 };
    float src1[3] = { 1,1,1 };

    if (s.source_rgb[0] == GL_CONSTANT) {
        src0[0] = s.env_color[0]; src0[1] = s.env_color[1]; src0[2] = s.env_color[2];
        if (s.operand_rgb[0] == GL_ONE_MINUS_SRC_COLOR) {
            src0[0] = 1.0f - src0[0]; src0[1] = 1.0f - src0[1]; src0[2] = 1.0f - src0[2];
        }
    }
    if (s.source_rgb[1] == GL_CONSTANT) {
        src1[0] = s.env_color[0]; src1[1] = s.env_color[1]; src1[2] = s.env_color[2];
        if (s.operand_rgb[1] == GL_ONE_MINUS_SRC_COLOR) {
            src1[0] = 1.0f - src1[0]; src1[1] = 1.0f - src1[1]; src1[2] = 1.0f - src1[2];
        }
    }

    switch (s.combine_rgb) {
    case GL_MODULATE:
        s.pass_mode[s.pass_count] = GL_MODULATE;
        s.pass_color[s.pass_count][0] = src1[0];
        s.pass_color[s.pass_count][1] = src1[1];
        s.pass_color[s.pass_count][2] = src1[2];
        s.pass_color[s.pass_count][3] = 1.0f;
        s.pass_factor[s.pass_count] = 1.0f;
        s.pass_count++;
        break;
    case GL_ADD:
        s.pass_mode[s.pass_count] = GL_ADD;
        s.pass_color[s.pass_count][0] = src1[0];
        s.pass_color[s.pass_count][1] = src1[1];
        s.pass_color[s.pass_count][2] = src1[2];
        s.pass_color[s.pass_count][3] = 1.0f;
        s.pass_factor[s.pass_count] = 1.0f;
        s.pass_count++;
        break;
    case GL_INTERPOLATE: {
        float fac = (src1[0] + src1[1] + src1[2]) / 3.0f;
        s.pass_mode[s.pass_count] = GL_INTERPOLATE;
        s.pass_color[s.pass_count][0] = src0[0];
        s.pass_color[s.pass_count][1] = src0[1];
        s.pass_color[s.pass_count][2] = src0[2];
        s.pass_color[s.pass_count][3] = 1.0f;
        s.pass_factor[s.pass_count] = fac;
        s.pass_count++;
        break;
    }
    case GL_REPLACE:
    default:
        s.pass_mode[s.pass_count] = GL_REPLACE;
        s.pass_color[s.pass_count][0] = src1[0];
        s.pass_color[s.pass_count][1] = src1[1];
        s.pass_color[s.pass_count][2] = src1[2];
        s.pass_color[s.pass_count][3] = 1.0f;
        s.pass_factor[s.pass_count] = 1.0f;
        s.pass_count++;
        break;
    }
}

static void I_SectorCombinerUniforms(void) {
    if (!g_procs_loaded) 
        return;

    GLint curProg = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &curProg);
    if (curProg == 0) 
        return;

    GLint locUseTex = pglGetUniformLocation(curProg, "uUseTex");
    GLint locTexel = pglGetUniformLocation(curProg, "uTexel");
    GLint locPassCount = pglGetUniformLocation(curProg, "uPassCount");
    GLint locFogEn = pglGetUniformLocation(curProg, "uFogEnabled");
    GLint locFogCol = pglGetUniformLocation(curProg, "uFogColor");
    GLint locFogFac = pglGetUniformLocation(curProg, "uFogFactor");

        GLint locFogMode = pglGetUniformLocation(curProg, "uFogMode");
    GLint locFogStart = pglGetUniformLocation(curProg, "uFogStart");
    GLint locFogEnd = pglGetUniformLocation(curProg, "uFogEnd");
    GLint locFogDensity = pglGetUniformLocation(curProg, "uFogDensity");
if (locUseTex != -1) 
        pglUniform1i(locUseTex, s.use_texture ? 1 : 0);
    if (locTexel != -1) {
        if (s.tex_w > 0 && s.tex_h > 0) 
            pglUniform2f(locTexel, 1.0f / (float)s.tex_w, 1.0f / (float)s.tex_h);
        else
            pglUniform2f(locTexel, 0.0f, 0.0f);
    }
    if (locPassCount != -1) 
        pglUniform1i(locPassCount, s.pass_count);

    for (int i = 0; i < s.pass_count && i < MAX_PASSES; i++) {
        char n1[32], n2[32], n3[32];
        SDL_snprintf(n1, sizeof(n1), "uPassMode[%d]", i);
        SDL_snprintf(n2, sizeof(n2), "uPassColor[%d]", i);
        SDL_snprintf(n3, sizeof(n3), "uPassFactor[%d]", i);
        GLint m = pglGetUniformLocation(curProg, n1);
        GLint c = pglGetUniformLocation(curProg, n2);
        GLint f = pglGetUniformLocation(curProg, n3);
        if (m != -1) pglUniform1i(m, s.pass_mode[i]);
        if (c != -1) pglUniform4f(c, s.pass_color[i][0], s.pass_color[i][1], s.pass_color[i][2], s.pass_color[i][3]);
        if (f != -1) pglUniform1f(f, s.pass_factor[i]);
    }

    if (locFogEn != -1) 
        pglUniform1i(locFogEn, s.fog_enabled ? 1 : 0);
    if (locFogCol != -1) 
        pglUniform3f(locFogCol, s.fog_color[0], s.fog_color[1], s.fog_color[2]);
    if (locFogFac != -1) 
        pglUniform1f(locFogFac, s.fog_factor);
    if (locFogMode != -1)
        pglUniform1i(locFogMode, s.fog_mode);
    if (locFogStart != -1)
        pglUniform1f(locFogStart, s.fog_start);
    if (locFogEnd != -1)
        pglUniform1f(locFogEnd, s.fog_end);
    if (locFogDensity != -1)
        pglUniform1f(locFogDensity, s.fog_density);
}


void I_SectorCombiner_Init(void) { 
    load_glsl_procs(); 
    reset_state(); 
}
int  I_SectorCombiner_IsReady(void) { 
    return g_procs_loaded; 
}
void I_SectorCombiner_Bind(int useTex, int w, int h) { 
    s.use_texture = useTex ? 1 : 0; 
    s.tex_w = w; 
    s.tex_h = h; 
    I_SectorCombinerTexturePasses();
    I_SectorCombinerUniforms();
}
void I_SectorCombiner_Unbind(void) { /* no-op */ }

void I_SectorCombiner_SetFogParams(int mode, float start, float end, float density) { 
    s.fog_mode = mode; s.fog_start = start; s.fog_end = end; s.fog_density = density;
    I_SectorCombinerUniforms();
}
void I_SectorCombiner_SetEnvColor(float r, float g, float b, float a) { 
    s.env_color[0] = r; 
    s.env_color[1] = g; 
    s.env_color[2] = b; 
    s.env_color[3] = a; 
    I_SectorCombinerTexturePasses();
    I_SectorCombinerUniforms();
}
void I_SectorCombiner_SetCombineRGB(int mode) { 
    s.combine_rgb = mode; 
    I_SectorCombinerTexturePasses();
    I_SectorCombinerUniforms();
}
void I_SectorCombiner_SetCombineAlpha(int mode) { 
    s.combine_alpha = mode;
}
void I_SectorCombiner_SetSourceRGB(int slot, int src) { 
    if (slot >= 0 && slot < 3) 
        s.source_rgb[slot] = src; 
    I_SectorCombinerTexturePasses();
    I_SectorCombinerUniforms();
}
void I_SectorCombiner_SetOperandRGB(int slot, int op) { 
    if (slot >= 0 && slot < 3) 
        s.operand_rgb[slot] = op; 
    I_SectorCombinerTexturePasses();
    I_SectorCombinerUniforms();
}
void I_SectorCombiner_SetFog(int en, float r, float g, float b, float fac) {
    s.fog_enabled = en ? 1 : 0; s.fog_color[0] = r; s.fog_color[1] = g; s.fog_color[2] = b; s.fog_factor = fac;
    I_SectorCombinerUniforms();
}
void I_SectorCombiner_Commit(void) { 
    I_SectorCombinerTexturePasses();
    I_SectorCombinerUniforms();
}
