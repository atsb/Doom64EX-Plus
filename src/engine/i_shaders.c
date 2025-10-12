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

extern void I_SectorCombiner_Commit(void);

CVAR_EXTERNAL(r_filter);

static GLint sLocTexel = -1;
static GLint sLocUseTex = -1;

static GLuint generic_tint_overlay_prog = 0;

static GLint sLocPassCount = -1;
static GLint sLocPassMode[4] = { -1,-1,-1,-1 };
static GLint sLocPassColor[4] = { -1,-1,-1,-1 };
static GLint sLocPassFactor[4] = { -1,-1,-1,-1 };
static GLint sLocFogEnabled = -1, sLocFogColor = -1, sLocFogFactor = -1;

typedef struct {
	int combine_rgb;
	int combine_alpha;
	int source_rgb[3];
	int operand_rgb[3];
	float env_color[4];
	int pass_mode[4];
	float pass_color[4][4];
	float pass_factor[4];
	int pass_count;
	int fog_enabled; float fog_color[3]; float fog_factor;
} comb_state_t;

static struct {
	GLuint prog;
	GLint  locTex;
	int    initialised;
} shader_struct = { 0, -1, 0 };

static comb_state_t gComb;
static GLint  generic_tint_overlay_colour = -1;

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

static int is_glsl_loaded = 0;
static GLuint is_current_prog = 0;

static void I_ShaderLoad(void) {
	if (is_glsl_loaded) 
		return;
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
	is_glsl_loaded = 1;
}

/* BILATERAL AND N64 SHADERS
============================
*/

static const char* vertex_shader_bilateral =
"#version 120\n"
"varying vec2 vUV;\n"
"varying vec4 vColor;\nvarying float vEyeDist;\n"
"uniform int   uPassCount;\n"
"uniform int   uPassMode[4];\n"
"uniform vec4  uPassColor[4];\n"
"uniform float uPassFactor[4];\n"
"uniform int   uFogEnabled;\n"
"uniform vec3  uFogColor;\n"
"uniform float uFogFactor;\n"
"vec3 _applyPass(int mode, vec3 base, vec3 src, float f){\n""  if (mode==8448) return base*src;\n""  if (mode==260)  return base+src;\n""  if (mode==34165) return mix(base,src, clamp(f,0.0,1.0));\n""  if (mode==7681) return src;\n""  return base;\n""}\n""void main(){\n"
"  vec4 eye = gl_ModelViewMatrix * gl_Vertex;\n  vEyeDist = length(eye.xyz);\n  gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
"  vUV = gl_MultiTexCoord0.xy;\n"
"  vColor = gl_Color;\n"
"}\n";

/* N64 3-point filter (atsb) */
static const char* fragment_shader_bilateral_3point =
"#version 120\n"
"#define ADD_SCALE 0.60\n"
"uniform sampler2D uTex;\n"
"uniform vec2  uTexel;\n"
"uniform float uStrength;\n"
"uniform float uBleed;\n"
"uniform float uSeamFix;\n"
"uniform float uSnap;\n"
"uniform int   uUseTex;\n"
"varying vec2 vUV;\n"
"varying vec4 vColor;\nvarying float vEyeDist;\n"
"float _lum(vec3 c){ return dot(c, vec3(0.2126,0.7152,0.0722)); }\n"
"uniform int   uPassCount;\n"
"uniform int   uPassMode[4];\n"
"uniform vec4  uPassColor[4];\n"
"uniform float uPassFactor[4];\n"
"uniform int   uFogEnabled;\n"
"uniform vec3  uFogColor;\n"
"uniform float uFogFactor;\nuniform int   uFogMode;\nuniform float uFogStart;\nuniform float uFogEnd;\nuniform float uFogDensity;\n"
"\n"
"vec3 apply_combiner(vec3 rgb){\n"
"  for (int i=0;i<4;i++){\n"
"    if (i>=uPassCount) break;\n"
"    vec3  src = uPassColor[i].rgb;\n"
"    float fac = uPassFactor[i];\n"
"    int   mode= uPassMode[i];\n"
"    if (fac < 0.0 && fac > -1.5) fac = _lum(vColor.rgb);\n"
"    \n"
"    if (mode == 8448) {                           // GL_MODULATE\n"
"      rgb *= src;\n"
"    } else if (mode == 260) {                     // GL_ADD\n"
"      float amp = clamp(abs(fac),0.0,1.0)*ADD_SCALE;\n"
"      amp = min(amp*0.8, max(0.0, 1.0 - _lum(rgb)) + 1e-5);\n"
"      if (fac < -0.5) {                           // colored ADD\n"
"        float m = max(max(vColor.r,vColor.g),vColor.b);\n"
"        vec3 tint = (m > 1e-5) ? (vColor.rgb/m) : vColor.rgb;\n"
"        rgb += tint * src * amp;\n"
"      } else {\n"
"        rgb += src * amp;                         // plain scaled ADD\n"
"      }\n"
"    } else if (mode == 34165) {                   // GL_INTERPOLATE\n"
"      rgb = mix(rgb, src, clamp(fac,0.0,1.0));\n"
"    } else if (mode == 7681) {                    // GL_REPLACE\n"
"      rgb = src;\n"
"    }\n"
"  }\n"
"  return rgb;\n"
"}\n"
"\n"
"vec3 enhance_colors(vec3 rgb){\n"
"  float l = dot(rgb, vec3(0.2126, 0.7152, 0.0722));\n"
"  rgb = mix(vec3(l), rgb, 1.02);  // saturation\n"
"  rgb.r = rgb.r * 1.08;\n"
"  rgb.g = rgb.g * 1.08;\n"
"  rgb.b = rgb.b * 1.08;\n"
"  rgb = pow(max(rgb, 0.0), vec3(0.94));\n"
"  return rgb;\n"
"}\n"
"\n"
"void main(){\n"
"  // 1: no texture\n"
"  if (uUseTex == 0) {\n"
"    vec4 col = vColor; vec3 rgb = apply_combiner(col.rgb);\n"
"    rgb = enhance_colors(rgb);\n"
"    if (uFogEnabled!=0) {\n    float fogF = 1.0;\n    if (uFogMode == 1) {\n        float denom = max(uFogEnd - uFogStart, 0.0001);\n        fogF = clamp((uFogEnd - vEyeDist) / denom, 0.0, 1.0);\n    } else if (uFogMode == 2) {\n        fogF = clamp(exp(-uFogDensity * vEyeDist), 0.0, 1.0);\n    } else if (uFogMode == 3) {\n        float d = uFogDensity * vEyeDist;\n        fogF = clamp(exp(-(d*d)), 0.0, 1.0);\n    } else {\n        fogF = clamp(1.0 - uFogFactor, 0.0, 1.0);\n    }\n    rgb = mix(uFogColor, rgb, fogF);\n}\n"
"    gl_FragColor = vec4(clamp(rgb,0.0,1.0), col.a); return; }\n"
"\n"
"  // 2: texture but no 3-point\n"
"  if (uTexel.x <= 0.0 || uTexel.y <= 0.0) {\n"
"    vec4 col = texture2D(uTex, vUV) * vColor; vec3 rgb = apply_combiner(col.rgb);\n"
"    rgb = enhance_colors(rgb);\n"
"    if (uFogEnabled!=0) {\n    float fogF = 1.0;\n    if (uFogMode == 1) {\n        float denom = max(uFogEnd - uFogStart, 0.0001);\n        fogF = clamp((uFogEnd - vEyeDist) / denom, 0.0, 1.0);\n    } else if (uFogMode == 2) {\n        fogF = clamp(exp(-uFogDensity * vEyeDist), 0.0, 1.0);\n    } else if (uFogMode == 3) {\n        float d = uFogDensity * vEyeDist;\n        fogF = clamp(exp(-(d*d)), 0.0, 1.0);\n    } else {\n        fogF = clamp(1.0 - uFogFactor, 0.0, 1.0);\n    }\n    rgb = mix(uFogColor, rgb, fogF);\n}\n"
"    gl_FragColor = vec4(clamp(rgb,0.0,1.0), col.a); return; }\n"
"\n"
"  // 3: N64 3-point\n"
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
"\n"
"  vec4 col = tex * vColor; vec3 rgb = apply_combiner(col.rgb);\n"
"  rgb = enhance_colors(rgb);\n"
"  if (uFogEnabled!=0) {\n    float fogF = 1.0;\n    if (uFogMode == 1) {\n        float denom = max(uFogEnd - uFogStart, 0.0001);\n        fogF = clamp((uFogEnd - vEyeDist) / denom, 0.0, 1.0);\n    } else if (uFogMode == 2) {\n        fogF = clamp(exp(-uFogDensity * vEyeDist), 0.0, 1.0);\n    } else if (uFogMode == 3) {\n        float d = uFogDensity * vEyeDist;\n        fogF = clamp(exp(-(d*d)), 0.0, 1.0);\n    } else {\n        fogF = clamp(1.0 - uFogFactor, 0.0, 1.0);\n    }\n    rgb = mix(uFogColor, rgb, fogF);\n}\n"
"  gl_FragColor = vec4(clamp(rgb,0.0,1.0), col.a);\n"
"}\n";

/* atsb: bilinear */
static const char* fragment_shader_bilateral =
"#version 120\n"
"#define ADD_SCALE 0.60\n"
"uniform sampler2D uTex;\n"
"varying vec2 vUV;\n"
"varying vec4 vColor;\nvarying float vEyeDist;\n"
"float _lum(vec3 c){ return dot(c, vec3(0.2126,0.7152,0.0722)); }\n"
"uniform int   uPassCount;\n"
"uniform int   uPassMode[4];\n"
"uniform vec4  uPassColor[4];\n"
"uniform float uPassFactor[4];\n"
"uniform int   uFogEnabled;\n"
"uniform vec3  uFogColor;\n"
"uniform float uFogFactor;\nuniform int   uFogMode;\nuniform float uFogStart;\nuniform float uFogEnd;\nuniform float uFogDensity;\n"
"\n"
"vec3 apply_combiner(vec3 rgb){\n"
"  for (int i=0;i<4;i++){\n"
"    if (i>=uPassCount) break;\n"
"    vec3  src = uPassColor[i].rgb;\n"
"    float fac = uPassFactor[i];\n"
"    int   mode= uPassMode[i];\n"
"    if (fac < 0.0 && fac > -1.5) fac = _lum(vColor.rgb);\n"
"    \n"
"    if (mode == 8448) {                           // GL_MODULATE\n"
"      rgb *= src;\n"
"    } else if (mode == 260) {                     // GL_ADD\n"
"      float amp = clamp(abs(fac),0.0,1.0)*ADD_SCALE;\n"
"      amp = min(amp*0.8, max(0.0, 1.0 - _lum(rgb)) + 1e-5);\n"
"      if (fac < -0.5) {                           // colored ADD\n"
"        float m = max(max(vColor.r,vColor.g),vColor.b);\n"
"        vec3 tint = (m > 1e-5) ? (vColor.rgb/m) : vColor.rgb;\n"
"        rgb += tint * src * amp;\n"
"      } else {\n"
"        rgb += src * amp;                         // plain scaled ADD\n"
"      }\n"
"    } else if (mode == 34165) {                   // GL_INTERPOLATE\n"
"      rgb = mix(rgb, src, clamp(fac,0.0,1.0));\n"
"    } else if (mode == 7681) {                    // GL_REPLACE\n"
"      rgb = src;\n"
"    }\n"
"  }\n"
"  return rgb;\n"
"}\n"
"\n"
"vec3 enhance_colors(vec3 rgb){\n"
"  float l = dot(rgb, vec3(0.2126, 0.7152, 0.0722));\n"
"  rgb = mix(vec3(l), rgb, 1.02);  // saturation\n"
"  rgb.r = rgb.r * 1.08;\n"
"  rgb.g = rgb.g * 1.08;\n"
"  rgb.b = rgb.b * 1.08;\n"
"  rgb = pow(max(rgb, 0.0), vec3(0.94));\n"
"  return rgb;\n"
"}\n"
"\n"
"void main(){\n"
"  vec4 col = texture2D(uTex, vUV) * vColor;\n"
"  vec3 rgb = apply_combiner(col.rgb);\n"
"  rgb = enhance_colors(rgb);\n"
"  if (uFogEnabled!=0) {\n    float fogF = 1.0;\n    if (uFogMode == 1) {\n        float denom = max(uFogEnd - uFogStart, 0.0001);\n        fogF = clamp((uFogEnd - vEyeDist) / denom, 0.0, 1.0);\n    } else if (uFogMode == 2) {\n        fogF = clamp(exp(-uFogDensity * vEyeDist), 0.0, 1.0);\n    } else if (uFogMode == 3) {\n        float d = uFogDensity * vEyeDist;\n        fogF = clamp(exp(-(d*d)), 0.0, 1.0);\n    } else {\n        fogF = clamp(1.0 - uFogFactor, 0.0, 1.0);\n    }\n    rgb = mix(uFogColor, rgb, fogF);\n}\n"
"  gl_FragColor = vec4(clamp(rgb,0.0,1.0), col.a);\n"
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
	if (shader_struct.initialised) 
		return;
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
	sLocPassCount = pglGetUniformLocation(shader_struct.prog, "uPassCount");
	for (int i=0;i<4;i++) {
		char n1[32], n2[32], n3[32];
		SDL_snprintf(n1,sizeof(n1),"uPassMode[%d]",i);
		SDL_snprintf(n2,sizeof(n2),"uPassColor[%d]",i);
		SDL_snprintf(n3,sizeof(n3),"uPassFactor[%d]",i);
		sLocPassMode[i]  = pglGetUniformLocation(shader_struct.prog, n1);
		sLocPassColor[i] = pglGetUniformLocation(shader_struct.prog, n2);
		sLocPassFactor[i]= pglGetUniformLocation(shader_struct.prog, n3);
	}
	sLocFogEnabled = pglGetUniformLocation(shader_struct.prog, "uFogEnabled");
	sLocFogColor   = pglGetUniformLocation(shader_struct.prog, "uFogColor");
	sLocFogFactor  = pglGetUniformLocation(shader_struct.prog, "uFogFactor");

	shader_struct.initialised = 1;

	// init combiner defaults
	gComb.combine_rgb = 8448; gComb.combine_alpha = 8448;
	for (int i=0;i<3;i++){ 
		gComb.source_rgb[i]=0x1702; 
		gComb.operand_rgb[i]=0x0300; 
	}
	gComb.env_color[0]=gComb.env_color[1]=gComb.env_color[2]=0.0f; gComb.env_color[3]=1.0f;
	gComb.pass_count=0; gComb.fog_enabled=0; gComb.fog_color[0]=gComb.fog_color[1]=gComb.fog_color[2]=0.0f; gComb.fog_factor=0.0f;

	// push combiner to shader
	if (sLocPassCount>=0) 
		pglUniform1i(sLocPassCount, 0);
	if (sLocFogEnabled>=0) 
		pglUniform1i(sLocFogEnabled, 0);
	if (sLocFogColor>=0) 
		pglUniform3f(sLocFogColor,0.0f,0.0f,0.0f);
	if (sLocFogFactor>=0) 
		pglUniform1f(sLocFogFactor,0.0f);

}

/* GENERIC TINT OVERLAY SHADERS
===============================
*/

static GLuint I_OverlayTintShaderCompile(GLenum tp, const char* src) {
	GLuint sh = pglCreateShader(tp);
	pglShaderSource(sh, 1, &src, NULL);
	pglCompileShader(sh);
	return sh;
}

void I_OverlayTintShaderInit(void) {
	if (generic_tint_overlay_prog) 
		return;
	GLuint vs = I_OverlayTintShaderCompile(GL_VERTEX_SHADER,
		"#version 120\n"
		"void main(){ gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex; }\n");
	GLuint fs = I_OverlayTintShaderCompile(GL_FRAGMENT_SHADER,
		"#version 120\n"
		"uniform vec4 uColor;\n" 
		"void main(){ gl_FragColor = uColor; }\n");
	generic_tint_overlay_prog = pglCreateProgram();
	pglAttachShader(generic_tint_overlay_prog, vs);
	pglAttachShader(generic_tint_overlay_prog, fs);
	pglLinkProgram(generic_tint_overlay_prog);
	generic_tint_overlay_colour = pglGetUniformLocation(generic_tint_overlay_prog, "uColor");
}

void I_ShaderFullscreenTint(float r, float g, float b, float a) {
	if (a <= 0.0f)
		return;

	I_OverlayTintShaderInit();

	if (!(generic_tint_overlay_prog && generic_tint_overlay_colour >= 0))
		return;

	GLint oldProg = 0; glGetIntegerv(GL_CURRENT_PROGRAM, &oldProg);

	GLboolean wasBlend = glIsEnabled(GL_BLEND);
	GLboolean wasTex2D = glIsEnabled(GL_TEXTURE_2D);
	GLboolean wasDepthTest = glIsEnabled(GL_DEPTH_TEST);
	GLboolean wasAlphaTest = glIsEnabled(GL_ALPHA_TEST);

	GLint oldSrc = 0, oldDst = 0; glGetIntegerv(GL_BLEND_SRC, &oldSrc);
	glGetIntegerv(GL_BLEND_DST, &oldDst);
	GLboolean oldDepthMask; glGetBooleanv(GL_DEPTH_WRITEMASK, &oldDepthMask);

	pglUseProgram(generic_tint_overlay_prog);
	pglUniform4f(generic_tint_overlay_colour, r, g, b, a);

	GL_SetOrtho(1);
	if (!wasBlend) glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	if (wasTex2D)
		glDisable(GL_TEXTURE_2D);

	if (wasAlphaTest)
		glDisable(GL_ALPHA_TEST);

	if (wasDepthTest)
		glDisable(GL_DEPTH_TEST);

	glDepthMask(GL_FALSE);

	glColor4ub(255, 255, 255, 255);
	dglRecti(SCREENWIDTH, SCREENHEIGHT, 0, 0);

	glDepthMask(oldDepthMask);

	if (wasDepthTest)
		glEnable(GL_DEPTH_TEST);
	else
		glDisable(GL_DEPTH_TEST);

	if (wasAlphaTest)
		glEnable(GL_ALPHA_TEST);

	if (wasTex2D)
		glEnable(GL_TEXTURE_2D);
	glBlendFunc(oldSrc, oldDst);

	if (!wasBlend)
		glDisable(GL_BLEND);
	GL_ResetViewport();

	pglUseProgram(oldProg);
}

/* SHADER FUNCTIONS USED THROUGHOUT THE CODE
============================================
*/

void I_ShaderBind(void) {
	I_3PointShaderInit();
	I_OverlayTintShaderInit();
	if (is_current_prog != shader_struct.prog) {
		pglUseProgram(shader_struct.prog);
		is_current_prog = shader_struct.prog;
	}
	if (shader_struct.locTex >= 0)
		pglUniform1i(shader_struct.locTex, 0);
	if (sLocUseTex >= 0)
		pglUniform1i(sLocUseTex, 1);
	if (sLocTexel >= 0)
		pglUniform2f(sLocTexel, 0.0f, 0.0f);
	I_SectorCombiner_Commit();
}

void I_ShaderUnBind(void) {
	if (!shader_struct.initialised)
		return;
	if (is_current_prog != 0) {
		pglUseProgram(0);
		is_current_prog = 0;
	}
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
