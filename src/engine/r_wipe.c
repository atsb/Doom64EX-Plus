// Emacs style mode select   -*- C -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 1997 Midway Home Entertainment, Inc
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
// DESCRIPTION: Endlevel wipe FX.
//
//-----------------------------------------------------------------------------

#include "r_wipe.h"
#include "doomdef.h"
#include "i_system.h"
#include "gl_texture.h"
#include "doomstat.h"
#include "dgl.h"
#include "m_menu.h"

static dtexture wipeMeltTexture = 0;
static int wipeFadeAlpha = 0;

//
// WIPE_DisplayScreen
//

static void WIPE_RefreshDelay(void) {
	int starttime = I_GetTime();
	int tics = 0;

	do {
		tics = I_GetTime() - starttime;
		//
		// don't bash the CPU
		//
		I_Sleep(1);
	} while (!tics);
}

//
// WIPE_FadeScreen
//

void WIPE_FadeScreen(int fadetics) {
	int padw, padh;
	vtx_t v[4];
	float left, right, top, bottom;

	I_ShaderUnBind();

	allowmenu = false;

	wipeFadeAlpha = 0xff;
	wipeMeltTexture = GL_ScreenToTexture();

	padw = GL_PadTextureDims(video_width);
	padh = GL_PadTextureDims(video_height);

	GL_SetState(GLSTATE_BLEND, 1);
	dglEnable(GL_TEXTURE_2D);

	//
	// setup vertex coordinates for plane
	//
	left = (float)(ViewWindowX * ViewWidth / video_width);
	right = left + (SCREENWIDTH * ViewWidth / video_width);
	top = (float)(ViewWindowY * ViewHeight / video_height);
	bottom = top + (SCREENHEIGHT * ViewHeight / video_height);

	v[0].x = v[2].x = left;
	v[1].x = v[3].x = right;
	v[0].y = v[1].y = top;
	v[2].y = v[3].y = bottom;

	v[0].z = v[1].z = v[2].z = v[3].z = 0.0f;

	v[0].tu = v[2].tu = 0.0f;
	v[1].tu = v[3].tu = (float)video_width / (float)padw;
	v[0].tv = v[1].tv = (float)video_height / (float)padh;
	v[2].tv = v[3].tv = 0.0f;

	dglBindTexture(GL_TEXTURE_2D, wipeMeltTexture);

	//
	// begin fade out
	//
	while (wipeFadeAlpha > 0) {
		rcolor color;

		//
		// clear frame
		//
		GL_ClearView(0xFF000000);

		if (wipeFadeAlpha < 0) {
			wipeFadeAlpha = 0;
		}

		//
		// display screen overlay
		//
		color = D_RGBA(wipeFadeAlpha, wipeFadeAlpha, wipeFadeAlpha, 0xff);

		dglSetVertexColor(v, color, 4);
		GL_Draw2DQuad(v, 1);

		GL_SwapBuffers();

		WIPE_RefreshDelay();

		wipeFadeAlpha -= fadetics;
	}

	GL_SetState(GLSTATE_BLEND, 0);
	GL_UnloadTexture(&wipeMeltTexture);

	allowmenu = true;

	I_ShaderBind();
}

//
// WIPE_MeltScreen
//

void WIPE_MeltScreen(void) {
	int padw, padh;
	vtx_t v[4];
	vtx_t v2[4];
	float left, right, top, bottom;
	int i = 0;

	I_ShaderUnBind();

	M_ClearMenus();
	allowmenu = false;

	wipeMeltTexture = GL_ScreenToTexture();

	padw = GL_PadTextureDims(video_width);
	padh = GL_PadTextureDims(video_height);

	GL_SetState(GLSTATE_BLEND, 1);
	dglEnable(GL_TEXTURE_2D);

	//
	// setup vertex coordinates for plane
	//
	left = (float)(ViewWindowX * ViewWidth / video_width);
	right = left + (SCREENWIDTH * ViewWidth / video_width);
	top = (float)(ViewWindowY * ViewHeight / video_height);
	bottom = top + (SCREENHEIGHT * ViewHeight / video_height);

	v[0].x = v[2].x = left;
	v[1].x = v[3].x = right;
	v[0].y = v[1].y = top;
	v[2].y = v[3].y = bottom;

	v[0].z = v[1].z = v[2].z = v[3].z = 0.0f;

	v[0].tu = v[2].tu = 0.0f;
	v[1].tu = v[3].tu = (float)video_width / (float)padw;
	v[0].tv = v[1].tv = (float)video_height / (float)padh;
	v[2].tv = v[3].tv = 0.0f;

	dmemcpy(v2, v, sizeof(vtx_t) * 4);

	dglBindTexture(GL_TEXTURE_2D, wipeMeltTexture);
	GL_SetTextureMode(GL_ADD);

	for (i = 0; i < 160; i += 2) {
		int j;

		GL_ClearView(0xFF000000);

		dglSetVertexColor(v2, D_RGBA(1, 0, 0, 0xff), 4);
		GL_Draw2DQuad(v2, 1);

		dglSetVertexColor(v, D_RGBA(0, 0, 0, 0x10), 4);
		GL_Draw2DQuad(v, 1);

		//
		// move screen down. without clearing the frame, we should
		// get a nice melt effect using the HOM effect
		//
		for (j = 0; j < 4; j++) {
			v[j].y += 0.5f;
		}

		//
		// update screen buffer
		//
		dglCopyTexSubImage2D(
			GL_TEXTURE_2D,
			0,
			0,
			0,
			0,
			0,
			padw,
			padh
		);

		GL_SwapBuffers();

		WIPE_RefreshDelay();
	}

	//
	// reset combiners and blending
	//
	GL_SetTextureMode(GL_MODULATE);
	GL_SetDefaultCombiner();
	GL_SetState(GLSTATE_BLEND, 0);
	GL_UnloadTexture(&wipeMeltTexture);

	//
	// fade screen out
	//
	WIPE_FadeScreen(6);

	I_ShaderBind();
}
