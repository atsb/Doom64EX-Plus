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

#include <SDL3/SDL.h>
#include "doomstat.h"
#include "con_cvar.h"
#include "i_shaders.h"
#include "dgl.h"

CVAR_EXTERNAL(r_filter);

static GLint sLocTexel = -1;
static GLint sLocUseTex = -1;
int g_berserkFlashTics = 0;

/* this is where shaders are defined */

static GLuint(APIENTRY* pglCreateShader)(GLenum);
static void   (APIENTRY* pglShaderSource)(GLuint, GLsizei, const GLchar* const*, const GLint*);
static void   (APIENTRY* pglCompileShader)(GLuint);
static void   (APIENTRY* pglGetShaderiv)(GLuint, GLenum, GLint*);
static void   (APIENTRY* pglGetShaderInfoLog)(GLuint, GLsizei, GLsizei*, GLchar*);
static GLuint(APIENTRY* pglCreateProgram)(void);
static void   (APIENTRY* pglAttachShader)(GLuint, GLuint);
static void   (APIENTRY* pglLinkProgram)(GLuint);
static void   (APIENTRY* pglGetProgramiv)(GLuint, GLenum, GLint*);
static void   (APIENTRY* pglGetProgramInfoLog)(GLuint, GLsizei, GLsizei*, GLchar*);
static void   (APIENTRY* pglUseProgram)(GLuint);
static GLint(APIENTRY* pglGetUniformLocation)(GLuint, const GLchar*);
static void   (APIENTRY* pglUniform1i)(GLint, int);
static void (APIENTRY* pglUniform2f)(GLint, float, float);
static void (APIENTRY* pglUniform1f)(GLint, float);
static void (APIENTRY* pglUniform1f)(GLint, float);
static void (APIENTRY* pglUniform2f)(GLint, float, float);
static void (APIENTRY* pglUniform3f)(GLint, float, float, float);
static void (APIENTRY* pglUniform4f)(GLint, float, float, float, float);

static void I_ShaderLoad(void) {
#define GL_GET(fn) *(void**)(&p##fn) = SDL_GL_GetProcAddress(#fn)
	GL_GET(glCreateShader);  GL_GET(glShaderSource);
	GL_GET(glCompileShader); GL_GET(glGetShaderiv);
	GL_GET(glGetShaderInfoLog);
	GL_GET(glCreateProgram); GL_GET(glAttachShader);
	GL_GET(glLinkProgram);   GL_GET(glGetProgramiv);
	GL_GET(glGetProgramInfoLog);
	GL_GET(glUseProgram);    GL_GET(glGetUniformLocation);
	GL_GET(glUniform1i);
	GL_GET(glUniform2f);
	GL_GET(glUniform1f);
	GL_GET(glUniform3f);
	GL_GET(glUniform4f);
#undef GL_GET
}

/* BILATERAL AND N64 SHADERS
============================
*/

static const char* vertex_shader_bilateral =
"#version 120\n"
"varying vec2 vUV;\n"
"varying vec4 vColor;\n"
"void main(){\n"
"  gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
"  vUV = gl_MultiTexCoord0.xy;\n"
"  vColor = gl_Color;\n"
"}\n";

/* N64 3-point filter (atsb) */
static const char* fragment_shader_bilateral_3point =
"#version 120\n"
"uniform sampler2D uTex;\n"
"uniform vec2  uTexel;\n"
"uniform float uStrength;\n"
"uniform float uBleed;\n"
"uniform float uSeamFix;\n"
"uniform float uSnap;\n"
"uniform int   uUseTex;\n"
"varying vec2 vUV;\n"
"varying vec4 vColor;\n"
"\n"
"void main(){\n"
"  if (uUseTex == 0) { gl_FragColor = vColor; return; }\n"
"\n"
"  if (uTexel.x <= 0.0 || uTexel.y <= 0.0) {\n"
"    gl_FragColor = texture2D(uTex, vUV) * vColor; return;\n"
"  }\n"
"\n"
"  vec2 texSize = 1.0 / uTexel;\n"
"  vec2 p  = vUV * texSize - 0.5;\n"
"  vec2 ip = floor(p);\n"
"  vec2 f  = p - ip;\n"
"  if (uSnap > 0.0) { vec2 q = vec2(uSnap); f = floor(f*q + 0.5)/q; }\n"
"  vec2 baseUV = (ip + 0.5) / texSize;\n"
"\n"
"  float S = (uStrength > 0.0) ? max(uStrength, 1.0) : 1.0;\n"
"  vec2 stepX  = vec2(uTexel.x * S, 0.0);\n"
"  vec2 stepY  = vec2(0.0,           uTexel.y * S);\n"
"  vec2 stepXY = stepX + stepY;\n"
"\n"
"  vec4 c00 = texture2D(uTex, baseUV);\n"
"  vec4 c10 = texture2D(uTex, baseUV + stepX);\n"
"  vec4 c01 = texture2D(uTex, baseUV + stepY);\n"
"  vec4 c11 = texture2D(uTex, baseUV + stepXY);\n"
"\n"
"  float s = f.x + f.y;\n"
"  vec4 triA = c00 + f.x*(c10-c00) + f.y*(c01-c00);\n"
"  vec4 triB = c11 + (1.0-f.x)*(c01-c11) + (1.0-f.y)*(c10-c11);\n"
"  float eps = max(uSeamFix, 0.0);\n"
"  float w   = (eps > 0.0) ? smoothstep(1.0 - eps, 1.0 + eps, s) : step(1.0, s);\n"
"  vec4 tex  = mix(triA, triB, w);\n"
"  if (uBleed > 0.0) { vec4 avg = 0.25*(c00+c10+c01+c11); tex = mix(tex, avg, clamp(uBleed,0.0,1.0)); }\n"
"  gl_FragColor = tex * vColor;\n"
"}\n";

static const char* fragment_shader_bilateral =
"#version 120\n"
"uniform sampler2D uTex;\n"
"varying vec2 vUV;\n"
"varying vec4 vColor;\n"
"\n"
"void main(){\n"
"  gl_FragColor = texture2D(uTex, vUV) * vColor;\n"
"}\n";

static GLuint I_3PointShaderCompile(GLenum type, const char* src) {
	GLuint shader = pglCreateShader(type);
	pglShaderSource(shader, 1, &src, NULL);
	pglCompileShader(shader);
	GLint ok = 0;
	pglGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
	if (!ok) {
		char log[1024]; GLsizei n = 0;
		pglGetShaderInfoLog(shader, sizeof(log), &n, log);
		fprintf(stderr, "I_3PointShaderCompile: %s compilation error:\\n%.*s\\n",
			type == GL_VERTEX_SHADER ? "vertex" : "fragment", (int)n, log);
	}
	return shader;
}

static void I_3PointShaderInit(void) {
	GLuint fs;
	I_ShaderLoad();
	GLuint vs = I_3PointShaderCompile(GL_VERTEX_SHADER, vertex_shader_bilateral);
	if (r_filter.value == 0) {
		fs = I_3PointShaderCompile(GL_FRAGMENT_SHADER, fragment_shader_bilateral_3point);
	}
	else {
		fs = I_3PointShaderCompile(GL_FRAGMENT_SHADER, fragment_shader_bilateral);
	}
	shader_struct.prog = pglCreateProgram();
	pglAttachShader(shader_struct.prog, vs);
	pglAttachShader(shader_struct.prog, fs);
	pglLinkProgram(shader_struct.prog);
	GLint ok = 0; pglGetProgramiv(shader_struct.prog, GL_LINK_STATUS, &ok);
	if (!ok) {
		char log[1024]; GLsizei n = 0; pglGetProgramInfoLog(shader_struct.prog, sizeof(log), &n, log);
		fprintf(stderr, "I_3PointShaderInit: Linkage error:\n%.*s\n", (int)n, log);
	}
	shader_struct.locTex = pglGetUniformLocation(shader_struct.prog, "uTex");
	sLocTexel = pglGetUniformLocation(shader_struct.prog, "uTexel");
	sLocUseTex = pglGetUniformLocation(shader_struct.prog, "uUseTex");
	shader_struct.initialised = 1;
}

/* BERSERK TINT OVERLAY SHADERS
===============================
*/

static GLuint berserk_overlay_prog = 0;
static GLint  berserk_overlay_tint_colour = -1;

static const char* vertex_berserk_tint =
"#version 120\n"
"void main(){\n"
"  gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
"}\n";

static const char* fragment_berserk_tint =
"#version 120\n"
"uniform vec4  uColor;   // rgba in 0..1, e.g. vec4(1,0,0,0.4)\n"
"void main(){\n"
"  gl_FragColor = uColor;\n"
"}\n";

static GLuint I_ShaderBerserkCompile(GLenum tp, const char* src) {
	GLuint sh = pglCreateShader(tp); pglShaderSource(sh, 1, &src, NULL);
	pglCompileShader(sh); GLint ok = 0; pglGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
	return sh;
}

static void I_ShaderBerserkInit(void) {
	if (berserk_overlay_prog) return;
	GLuint vs = I_ShaderBerserkCompile(GL_VERTEX_SHADER, vertex_berserk_tint);
	GLuint fs = I_ShaderBerserkCompile(GL_FRAGMENT_SHADER, fragment_berserk_tint);
	berserk_overlay_prog = pglCreateProgram();
	pglAttachShader(berserk_overlay_prog, vs);
	pglAttachShader(berserk_overlay_prog, fs);
	pglLinkProgram(berserk_overlay_prog);
	berserk_overlay_tint_colour = pglGetUniformLocation(berserk_overlay_prog, "uColor");
}

void I_ShaderBerserkRenderTint(float r, float g, float b, float a)
{
	if (a <= 0.0f) return;

	I_ShaderBerserkInit();

	GLint oldProg = 0; glGetIntegerv(GL_CURRENT_PROGRAM, &oldProg);

	GLboolean wasBlend = glIsEnabled(GL_BLEND);
	GLboolean wasTex2D = glIsEnabled(GL_TEXTURE_2D);
	GLboolean wasDepthTest = glIsEnabled(GL_DEPTH_TEST);
	GLboolean wasAlphaTest = glIsEnabled(GL_ALPHA_TEST);

	GLint oldBlendSrc = 0, oldBlendDst = 0;
	glGetIntegerv(GL_BLEND_SRC, &oldBlendSrc);
	glGetIntegerv(GL_BLEND_DST, &oldBlendDst);

	GLboolean oldDepthMask; glGetBooleanv(GL_DEPTH_WRITEMASK, &oldDepthMask);

	pglUseProgram(berserk_overlay_prog);
	if (berserk_overlay_tint_colour >= 0) pglUniform4f(berserk_overlay_tint_colour, r, g, b, a);

	GL_SetOrtho(1);

	if (!wasBlend) glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	if (wasTex2D)     glDisable(GL_TEXTURE_2D);
	if (wasAlphaTest) glDisable(GL_ALPHA_TEST);

	if (wasDepthTest) glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);

	glColor4ub(255, 255, 255, 255);
	dglRecti(SCREENWIDTH, SCREENHEIGHT, 0, 0);

	glDepthMask(oldDepthMask);
	if (wasDepthTest) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
	if (wasAlphaTest) glEnable(GL_ALPHA_TEST);
	if (wasTex2D)     glEnable(GL_TEXTURE_2D);
	glBlendFunc(oldBlendSrc, oldBlendDst);
	if (!wasBlend) glDisable(GL_BLEND);

	GL_ResetViewport();

	pglUseProgram(oldProg);
}

/* BERSERK TINT OVERLAY FUNCTIONS
=================================
*/

int I_GetBerserkFlashTics(void) {
	return g_berserkFlashTics;
}

void I_TickBerserkFlash(void) {
	if (g_berserkFlashTics > 0)
		g_berserkFlashTics--;
}
void I_StartBerserkFlash(void) {
	g_berserkFlashTics = BERSERK_FLASH_TICS;
}

/* SHADER FUNCTIONS USED THROUGHOUT THE CODE
============================================
*/

void I_ShaderBind(void) {
	I_3PointShaderInit();
	pglUseProgram(shader_struct.prog);
	if (shader_struct.locTex >= 0)
		pglUniform1i(shader_struct.locTex, 0);
	if (sLocTexel >= 0)
		pglUniform2f(sLocTexel, 0.0f, 0.0f);

	if (shader_struct.locTex >= 0)
		pglUniform1i(shader_struct.locTex, 0);

	if (sLocUseTex >= 0)
		pglUniform1i(sLocUseTex, 1);
	if (sLocTexel >= 0)
		pglUniform2f(sLocTexel, 0.0f, 0.0f);
}

void I_ShaderUnBind(void) {
	if (!shader_struct.initialised)
		return;
	pglUseProgram(0);
}

void I_ShaderSetTextureSize(int w, int h) {
	if (!shader_struct.initialised || sLocTexel < 0)
		return;
	if (w > 0 && h > 0)
		pglUniform2f(sLocTexel, 1.0f / (float)w, 1.0f / (float)h);
	else
		pglUniform2f(sLocTexel, 0.0f, 0.0f);
}

void I_ShaderSetUseTexture(int on) {
	if (!shader_struct.initialised || sLocUseTex < 0)
		return;
	pglUniform1i(sLocUseTex, on ? 1 : 0);
}
