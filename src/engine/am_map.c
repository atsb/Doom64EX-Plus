// Emacs style mode select   -*- C -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 1993-1997 Id Software, Inc.
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
// DESCRIPTION:  the automap code (new and improved)
//
//-----------------------------------------------------------------------------

#include <math.h>

#include "am_map.h"
#include "i_sdlinput.h"
#include "doomdef.h"
#include "p_local.h"
#include "t_bsp.h"
#include "m_fixed.h"
#include "doomstat.h"
#include "d_englsh.h"
#include "tables.h"
#include "am_draw.h"
#include "m_misc.h"
#include "gl_draw.h"
#include "g_actions.h"
#include "g_controls.h"
#include "r_lights.h"
#include "r_main.h"

// automap flags

enum {
	AF_PANLEFT = 1,
	AF_PANRIGHT = 2,
	AF_PANTOP = 4,
	AF_PANBOTTOM = 8,
	AF_ZOOMIN = 16,
	AF_ZOOMOUT = 32,
	AF_PANMODE = 64,
	AF_PANGAMEPAD = 128
};

// automap vars

int             amCheating = 0;        //villsa: no longer static..
boolean        automapactive = false;
fixed_t         automapx = 0;
fixed_t         automapy = 0;
fixed_t			automappanx = 0;
fixed_t			automappany = 0;
byte            amModeCycle = 0;        // textured or line mode?
int             followplayer = 1;        // specifies whether to follow the player around

static player_t* plr;                       // the player represented by an arrow
static boolean stopped = true;
static word     am_blink = 0;        // player arrow blink tics
static angle_t  automapangle = 0;
float    scale = 640.0f;   // todo: reset scale after changing levels
static fixed_t  am_box[4];                  // automap bounding box of level
static byte     am_flags;                   // action flags for automap. Mostly for controls
static fixed_t  mpanx = 0;
static fixed_t  mpany = 0;
static angle_t  autoprevangle = 0;
static fixed_t  automapprevx = 0;
static fixed_t  automapprevy = 0;
static fixed_t automap_prev_x_interpolation = 0;
static fixed_t automap_prev_y_interpolation = 0;
static angle_t automap_prev_ang_interpolation = 0;

static const float min_scale = 200.0f;
static const float max_scale = 2000.0f;

void AM_Start(void);

// automap cvars

CVAR(am_lines, 1);
CVAR(am_nodes, 0);
CVAR(am_ssect, 0);
CVAR(am_fulldraw, 0);
CVAR(am_showkeycolors, 0);
CVAR(am_showkeymarkers, 0);
CVAR(am_drawobjects, 0);
CVAR(am_overlay, 0);

extern fixed_t R_Interpolate(fixed_t ticframe, fixed_t updateframe, boolean enable);

CVAR_EXTERNAL(v_msensitivityx);
CVAR_EXTERNAL(v_msensitivityy);
CVAR_EXTERNAL(i_interpolateframes);

//
// CMD_Automap
//

static CMD(Automap) {
	if (gamestate != GS_LEVEL) {
		return;
	}

	if (!automapactive) {
		if (menuactive) return;
		AM_Start();
	}
	else {
		if (++amModeCycle >= 2) {
			amModeCycle = 0;
			AM_Stop();
		}
	}
}

//
// CMD_AutomapSetFlag
//

static CMD(AutomapSetFlag) {
	if (gamestate != GS_LEVEL) {
		return;
	}

	if (!automapactive) {
		return;
	}

	if (data & PCKF_UP) {
		int64_t flags = (data ^ PCKF_UP);

		am_flags &= ~flags;

		if (flags & (AF_PANMODE | AF_PANGAMEPAD)) {
			automappany = automappanx = 0;
		}
	}
	else {
		am_flags |= data;
	}
}

//
// CMD_AutomapFollow
//

static CMD(AutomapFollow) {
	if (gamestate != GS_LEVEL) {
		return;
	}

	if (!automapactive) {
		return;
	}

	followplayer = !followplayer;
	plr->message = followplayer ? AMSTR_FOLLOWON : AMSTR_FOLLOWOFF;

	if (!followplayer) {
		automapx = plr->mo->x;
		automapy = plr->mo->y;
		automapangle = plr->mo->angle;
	}
	else {
		automappany = automappanx = 0;
	}
}

//
// AM_Reset
//

void AM_Reset(void) {
	automapx = 0;
	automapy = 0;
	automappanx = 0;
	automappany = 0;
	amModeCycle = 0;
	followplayer = 1;
	stopped = true;
	am_blink = 0;
	automapangle = 0;
	scale = 640.0f;
	am_flags = 0;
	mpanx = 0;
	mpany = 0;
	autoprevangle = 0;
	automapprevx = 0;
	automapprevy = 0;
}

//
// AM_Stop
//

void AM_Stop(void) {
	automapactive = false;
	stopped = true;
	R_RefreshBrightness();
}

//
// AM_Start
//

void AM_Start(void) {
	int pnum;

	if (!stopped) {
		AM_Stop();
	}

	stopped = false;
	automapactive = true;
	amModeCycle = 0;
	am_flags = 0;
	am_blink = 0x5F | 0x100;

	// find player to center on initially
	if (!playeringame[pnum = consoleplayer]) {
		for (pnum = 0; pnum < MAXPLAYERS; pnum++) {
			if (playeringame[pnum]) {
				break;
			}
		}
	}

	plr = &players[pnum];

	automapx = plr->mo->x;
	automapy = plr->mo->y;
	automapangle = plr->mo->angle;
}

/* StevenSYS: I don't know if this is going to be used at some point, so I've commented it out instead of removing it
static bool AM_HandleGamepadEvent(const SDL_Event* e)
{
	if (!automapactive) return false;

	static float s_leftx = 0.0f, s_lefty = 0.0f;

	switch (e->type) {
	case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
		switch (e->gbutton.button) {
		case SDL_GAMEPAD_BUTTON_SOUTH:
			CMD_AutomapSetFlag(AF_PANGAMEPAD, NULL);
			return true;

		case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER:
			if (am_flags & AF_PANGAMEPAD) { CMD_AutomapSetFlag(AF_ZOOMIN, NULL);  return true; }
			break;

		case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER:
			if (am_flags & AF_PANGAMEPAD) { CMD_AutomapSetFlag(AF_ZOOMOUT, NULL); return true; }
			break;

		case SDL_GAMEPAD_BUTTON_DPAD_UP:
			if (am_flags & AF_PANGAMEPAD) { CMD_AutomapSetFlag(AF_PANTOP, NULL);    return true; }
			break;

		case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
			if (am_flags & AF_PANGAMEPAD) { CMD_AutomapSetFlag(AF_PANBOTTOM, NULL); return true; }
			break;

		case SDL_GAMEPAD_BUTTON_DPAD_LEFT:
			if (am_flags & AF_PANGAMEPAD) { CMD_AutomapSetFlag(AF_PANLEFT, NULL);   return true; }
			break;

		case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:
			if (am_flags & AF_PANGAMEPAD) { CMD_AutomapSetFlag(AF_PANRIGHT, NULL);  return true; }
			break;
		}
		break;

	case SDL_EVENT_GAMEPAD_BUTTON_UP:
		switch (e->gbutton.button) {
		case SDL_GAMEPAD_BUTTON_SOUTH:
			CMD_AutomapSetFlag(AF_PANGAMEPAD | PCKF_UP, NULL);
			return true;

		case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER:
			CMD_AutomapSetFlag(AF_ZOOMIN | PCKF_UP, NULL);  return true;
		case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER:
			CMD_AutomapSetFlag(AF_ZOOMOUT | PCKF_UP, NULL);  return true;

		case SDL_GAMEPAD_BUTTON_DPAD_UP:
			CMD_AutomapSetFlag(AF_PANTOP | PCKF_UP, NULL);    return true;
		case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
			CMD_AutomapSetFlag(AF_PANBOTTOM | PCKF_UP, NULL);    return true;
		case SDL_GAMEPAD_BUTTON_DPAD_LEFT:
			CMD_AutomapSetFlag(AF_PANLEFT | PCKF_UP, NULL);    return true;
		case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:
			CMD_AutomapSetFlag(AF_PANRIGHT | PCKF_UP, NULL);    return true;
		}
		break;

	case SDL_EVENT_GAMEPAD_AXIS_MOTION:
		if (!(am_flags & AF_PANGAMEPAD)) return false;
		if (e->gaxis.axis == SDL_GAMEPAD_AXIS_LEFTX) s_leftx = e->gaxis.value;
		if (e->gaxis.axis == SDL_GAMEPAD_AXIS_LEFTY) s_lefty = e->gaxis.value;

		const float dead = 0.15f;
		float x = (fabsf(s_leftx) > dead) ? s_leftx : 0.0f;
		float y = (fabsf(s_lefty) > dead) ? s_lefty : 0.0f;

		float sx = x * 32767.0f * v_msensitivityx.value / (max_scale / scale);
		float sy = y * 32767.0f * v_msensitivityx.value / (max_scale / scale);

		mpanx = (fixed_t)(sx * (float)FRACUNIT);
		mpany = (fixed_t)(sy * (float)FRACUNIT);

		return true;
	}

	return false;
}
*/

//
// AM_GetBounds
// Get a dynamically resizable bounding box
// of the automap
//

static void AM_GetBounds(void) {
	int i;
	fixed_t block[8];
	fixed_t x;
	fixed_t y;
	fixed_t x1;
	fixed_t y1;
	fixed_t x2;
	fixed_t y2;
	fixed_t bx;
	fixed_t by;

	// need to manually flip the origins based on player's angle otherwise
	// the bounding box will get screwed up.

	if (automapangle >= ANG90 && automapangle <= -ANG90) {
		block[0] = ((bmapwidth << MAPBLOCKSHIFT) + bmaporgx);
		block[1] = bmaporgy;
		block[2] = bmaporgx;
		block[3] = bmaporgy;
		block[4] = bmaporgx;
		block[5] = ((bmapheight << MAPBLOCKSHIFT) + bmaporgy);
		block[6] = ((bmapwidth << MAPBLOCKSHIFT) + bmaporgx);
		block[7] = ((bmapheight << MAPBLOCKSHIFT) + bmaporgy);
	}
	else {
		block[0] = bmaporgx;
		block[1] = ((bmapheight << MAPBLOCKSHIFT) + bmaporgy);
		block[2] = ((bmapwidth << MAPBLOCKSHIFT) + bmaporgx);
		block[3] = ((bmapheight << MAPBLOCKSHIFT) + bmaporgy);
		block[4] = ((bmapwidth << MAPBLOCKSHIFT) + bmaporgx);
		block[5] = bmaporgy;
		block[6] = bmaporgx;
		block[7] = bmaporgy;
	}

	M_ClearBox(am_box);

	for (i = 0; i < 8; i += 2) {
		bx = (block[i] - automapx) / FRACUNIT;
		by = (block[i + 1] - automapy) / FRACUNIT;

		x1 = bx * finecosine[(ANG90 - automapangle) >> ANGLETOFINESHIFT];
		y1 = by * finesine[(ANG90 - automapangle) >> ANGLETOFINESHIFT];
		x2 = bx * finesine[(ANG90 - automapangle) >> ANGLETOFINESHIFT];
		y2 = by * finecosine[(ANG90 - automapangle) >> ANGLETOFINESHIFT];

		x = (x1 - y1) + automapx;
		y = (x2 + y2) + automapy;

		M_AddToBox(am_box, x, y);
	}
}

//
// AM_Responder
// Handle events (user inputs) in automap mode
//

boolean AM_Responder(event_t* ev) {
	int rc = false;
	bool useHeld = (plr && (plr->cmd.buttons & BT_USE));

	if (automapactive && ((am_flags & AF_PANMODE) || useHeld)) {
		if (ev->type == ev_mouse) {
 			mpanx = (fixed_t)ev->data2;
			mpany = (fixed_t)ev->data3;
			rc = true;
		}
		else {
			if (!useHeld && ev->type == ev_mousedown && ev->data1) {
				if (fmod(ev->data1, 2.0) != 0.0) {
					am_flags &= ~AF_ZOOMOUT;
					am_flags |= AF_ZOOMIN;
					rc = true;
				}
				else if (fmod(floor(ev->data1 / 4.0), 2.0) != 0.0) {
					am_flags &= ~AF_ZOOMIN;
					am_flags |= AF_ZOOMOUT;
					rc = true;
				}
			}
			else {
				am_flags &= ~AF_ZOOMOUT;
				am_flags &= ~AF_ZOOMIN;
			}
		}
	}

	return rc;
}

//
// AM_Ticker
// Updates on Game Tick
//

void AM_Ticker(void) {
	float speed;
	fixed_t oldautomapx;
	fixed_t oldautomapy;

	bool useHeld = (plr && (plr->cmd.buttons & BT_USE));

	if (!automapactive)
		return;

	AM_GetBounds();

	if (am_flags & AF_ZOOMOUT) {
		scale += 32.0f;
		if (scale > max_scale) scale = max_scale;
	}
	if (am_flags & AF_ZOOMIN) {
		scale -= 32.0f;
		if (scale < min_scale) scale = min_scale;
	}

	speed = (scale / 16.0f) * FRACUNIT;
	oldautomapx = automappanx;
	oldautomapy = automappany;

	bool updated_follow_target = false;

	if (followplayer) {
		if ((am_flags & AF_PANMODE) || useHeld) {
			float panscalex = v_msensitivityx.value / ((max_scale / 2) / scale);
			float panscaley = v_msensitivityy.value / ((max_scale / 2) / scale);

			fixed_t dx = (fixed_t)((I_MouseAccel((float)mpanx) * panscalex) * (FRACUNIT / 128.0f));
			fixed_t dy = (fixed_t)((I_MouseAccel((float)mpany) * panscaley) * (FRACUNIT / 128.0f));

			automappanx += dx;
			automappany += dy;

			mpanx = mpany = 0;
		}
		else {
			if (i_interpolateframes.value) {
				automap_prev_x_interpolation = automapx;
				automap_prev_y_interpolation = automapy;
				automap_prev_ang_interpolation = automapangle;
			}
			automapx = plr->mo->x;
			automapy = plr->mo->y;
			automapangle = plr->mo->angle;
			updated_follow_target = true;
		}
	}

	if (am_flags & AF_PANGAMEPAD) {
		automappanx += mpanx;
		automappany += mpany;
		mpanx = mpany = 0;
	}
	else if (!updated_follow_target) {
		if (i_interpolateframes.value) {
			automap_prev_x_interpolation = automapx;
			automap_prev_y_interpolation = automapy;
			automap_prev_ang_interpolation = automapangle;
		}
		automapx = plr->mo->x;
		automapy = plr->mo->y;
		automapangle = plr->mo->angle;
		updated_follow_target = true;
	}

	if ((!followplayer || (am_flags & AF_PANGAMEPAD)) &&
		(am_flags & (AF_PANLEFT | AF_PANRIGHT | AF_PANTOP | AF_PANBOTTOM))) {
		if (am_flags & AF_PANTOP)    automappany += (fixed_t)speed;
		if (am_flags & AF_PANLEFT)   automappanx -= (fixed_t)speed;
		if (am_flags & AF_PANRIGHT)  automappanx += (fixed_t)speed;
		if (am_flags & AF_PANBOTTOM) automappany -= (fixed_t)speed;
	}

	if (am_box[BOXRIGHT] < (automappanx + automapx) ||
		(automappanx + automapx) < am_box[BOXLEFT]) {
		automappanx = oldautomapx;
	}
	if (am_box[BOXTOP] < (automappany + automapy) ||
		(automappany + automapy) < am_box[BOXBOTTOM]) {
		automappany = oldautomapy;
	}

	if (am_blink & 0x100) {
		if ((am_blink & 0xff) == 0xff) am_blink = 0xff;
		else am_blink += 0x10;
	}
	else {
		if (am_blink < 0x5F) am_blink = 0x5F | 0x100;
		else am_blink -= 0x10;
	}
}

//
// AM_DrawMapped
//

static void AM_DrawMapped(void) {
	int i;

	//
	// draw textured subsectors for automap
	//
	AM_DrawLeafs(scale);

	//
	// draw white outlines around the subsectors for overlay mode
	//
	if (am_overlay.value) {
		fixed_t x1;
		fixed_t x2;
		fixed_t y1;
		fixed_t y2;
		seg_t* seg;
		int j;
		int p;

		for (j = 0; j < numsubsectors; j++) {
			if (subsectors[j].sector->flags & MS_HIDESSECTOR) {
				continue;
			}

			for (p = 0; p < subsectors[j].numlines; p++) {
				seg = &segs[subsectors[j].firstline + p];

				//
				// if the subsector has at least one seg that is mapped, then
				// draw the white outline for the entire subsector
				//
				if (seg->linedef->flags & ML_MAPPED ||
					(plr->powers[pw_allmap] || amCheating)) {
					for (i = 0; i < subsectors[j].numlines; i++) {
						seg = &segs[subsectors[j].firstline + i];

						if (!(seg->linedef->flags & ML_SECRET) && seg->linedef->backsector) {
							continue;
						}

						x1 = seg->linedef->v1->x;
						y1 = seg->linedef->v1->y;
						x2 = seg->linedef->v2->x;
						y2 = seg->linedef->v2->y;

						AM_DrawLine(x1, x2, y1, y2, scale, WHITE);
					}

					//
					// continue to next subsector
					//
					break;
				}
			}
		}
	}
}

//
// AM_DrawNodes
//

static void AM_DrawNodes(void) {
	int i;
	int x1, x2, y1, y2;
	node_t* node;
	int        side;
	int        nodenum;

	nodenum = numnodes - 1;

	if (am_nodes.value >= 4) {
		while (!(nodenum & NF_SUBSECTOR)) {
			node = &nodes[nodenum];
			side = R_PointOnSide(plr->mo->x, plr->mo->y, node);
			nodenum = node->children[side];
		}
	}

	for (i = 0; i < numnodes; i++) {
		if (am_nodes.value < 4) {
			node = &nodes[i];

			if (am_nodes.value == 1 || am_nodes.value >= 3) {
				x1 = node->bbox[0][BOXLEFT];
				y1 = node->bbox[0][BOXTOP];
				x2 = node->bbox[0][BOXRIGHT];
				y2 = node->bbox[0][BOXTOP];

				AM_DrawLine(x1, x2, y1, y2, scale, 0x00FFFFFF);

				x1 = node->bbox[0][BOXRIGHT];
				y1 = node->bbox[0][BOXTOP];
				x2 = node->bbox[0][BOXRIGHT];
				y2 = node->bbox[0][BOXBOTTOM];

				AM_DrawLine(x1, x2, y1, y2, scale, 0x00FFFFFF);

				x1 = node->bbox[0][BOXRIGHT];
				y1 = node->bbox[0][BOXBOTTOM];
				x2 = node->bbox[0][BOXLEFT];
				y2 = node->bbox[0][BOXBOTTOM];

				AM_DrawLine(x1, x2, y1, y2, scale, 0x00FFFFFF);

				x1 = node->bbox[0][BOXLEFT];
				y1 = node->bbox[0][BOXBOTTOM];
				x2 = node->bbox[0][BOXLEFT];
				y2 = node->bbox[0][BOXTOP];

				AM_DrawLine(x1, x2, y1, y2, scale, 0x00FFFFFF);

				x1 = node->bbox[1][BOXLEFT];
				y1 = node->bbox[1][BOXTOP];
				x2 = node->bbox[1][BOXRIGHT];
				y2 = node->bbox[1][BOXTOP];

				AM_DrawLine(x1, x2, y1, y2, scale, 0x00FF00FF);

				x1 = node->bbox[1][BOXRIGHT];
				y1 = node->bbox[1][BOXTOP];
				x2 = node->bbox[1][BOXRIGHT];
				y2 = node->bbox[1][BOXBOTTOM];

				AM_DrawLine(x1, x2, y1, y2, scale, 0x00FF00FF);

				x1 = node->bbox[1][BOXRIGHT];
				y1 = node->bbox[1][BOXBOTTOM];
				x2 = node->bbox[1][BOXLEFT];
				y2 = node->bbox[1][BOXBOTTOM];

				AM_DrawLine(x1, x2, y1, y2, scale, 0x00FF00FF);

				x1 = node->bbox[1][BOXLEFT];
				y1 = node->bbox[1][BOXBOTTOM];
				x2 = node->bbox[1][BOXLEFT];
				y2 = node->bbox[1][BOXTOP];

				AM_DrawLine(x1, x2, y1, y2, scale, 0x00FF00FF);
			}

			if (am_nodes.value == 2 || am_nodes.value >= 3) {
				x1 = node->x;
				y1 = node->y;
				x2 = (node->x + node->dx);
				y2 = (node->y + node->dy);

				AM_DrawLine(x1, x2, y1, y2, scale, 0xFFFF00FF);
			}
		}

		if (am_nodes.value >= 4) {
			break;
		}
	}
}

//
// AM_DrawWalls
// Determines visible lines, draws them.
// This is LineDef based, not LineSeg based.
//

static void AM_DrawWalls(void) {
	int i;
	int x1, x2, y1, y2;

	for (i = 0; i < numlines; i++) {
		line_t* l;

		l = &lines[i];

		x1 = l->v1->x;
		y1 = l->v1->y;
		x2 = l->v2->x;
		y2 = l->v2->y;

		//
		// 20120208 villsa - re-ordered flag checks to match original game
		//

		if (l->flags & ML_DONTDRAW) {
			continue;
		}

		if ((l->flags & ML_MAPPED) || am_fulldraw.value || plr->powers[pw_allmap] || amCheating) {
			rcolor color = D_RGBA(0x8A, 0x5C, 0x30, 0xFF);  // default color

			//
			// check for cheats
			//
			if ((plr->powers[pw_allmap] || amCheating) && !(l->flags & ML_MAPPED)) {
				color = D_RGBA(0x80, 0x80, 0x80, 0xFF);
			}
			//
			// check for secret line
			//
			else if (l->flags & ML_SECRET) {
				color = D_RGBA(0xA4, 0x00, 0x00, 0xFF);
			}
			//
			// handle special line
			//
			else if (l->special && !(l->flags & ML_HIDEAUTOMAPTRIGGER)) {
				//
				// draw colored doors based on key requirement
				//
				if (am_showkeycolors.value) {
					if (l->special & MLU_RED) {
						color = D_RGBA(0xFF, 0x00, 0x00, 0xFF);
					}
					else if (l->special & MLU_BLUE) {
						color = D_RGBA(0x00, 0x00, 0xFF, 0xFF);
					}
					else if (l->special & MLU_YELLOW) {
						color = D_RGBA(0xFF, 0xFF, 0x00, 0xFF);
					}
					else {
						//
						// change color to green to avoid confusion with yellow key doors
						//
						color = D_RGBA(0x00, 0xCC, 0x00, 0xFF);
					}
				}
				else {
					//
					// default color for special lines
					//
					color = D_RGBA(0xCC, 0xCC, 0x00, 0xFF);
				}
			}
			//
			// solid wall?
			//
			else if (!(l->flags & ML_TWOSIDED)) {
				color = D_RGBA(0xA4, 0x00, 0x00, 0xFF);
			}

			AM_DrawLine(x1, x2, y1, y2, scale, color);
		}
	}
}

//
// AM_drawPlayers
//
static void AM_drawPlayers(void) {
	int i;
	player_t* p_loop_player;
	byte flash;
	mobj_t* player_mobj;
	fixed_t original_x, original_y, render_x, render_y;
	angle_t original_angle, render_angle;

	flash = am_blink & 0xff;

	if (!netgame) {
		p_loop_player = plr;

		// Ensure player and its mobj are valid
		if (!p_loop_player || !p_loop_player->mo) return;
		player_mobj = p_loop_player->mo;

		if (i_interpolateframes.value) {
			render_x = R_Interpolate(player_mobj->x, player_mobj->frame_x, true);
			render_y = R_Interpolate(player_mobj->y, player_mobj->frame_y, true);
			render_angle = R_Interpolate(player_mobj->angle, player_mobj->angle, true);
		}
		else {
			render_x = player_mobj->x;
			render_y = player_mobj->y;
			render_angle = player_mobj->angle;
		}

		original_x = player_mobj->x;
		original_y = player_mobj->y;
		original_angle = player_mobj->angle;

		player_mobj->x = render_x;
		player_mobj->y = render_y;
		player_mobj->angle = render_angle;

		AM_DrawTriangle(player_mobj, scale, amModeCycle, 0, flash, 0);

		player_mobj->x = original_x;
		player_mobj->y = original_y;
		player_mobj->angle = original_angle;
		return;
	}

	for (i = 0; i < MAXPLAYERS; i++) {
		p_loop_player = &players[i];

		if ((deathmatch && !singledemo) && p_loop_player != plr) {
			continue;
		}
		if (!playeringame[i] || !p_loop_player->mo) {
			continue;
		}

		player_mobj = p_loop_player->mo;

		if (i_interpolateframes.value) {
			render_x = R_Interpolate(player_mobj->x, player_mobj->frame_x, true);
			render_y = R_Interpolate(player_mobj->y, player_mobj->frame_y, true);
			render_angle = R_Interpolate(player_mobj->angle, player_mobj->angle, true);
		}
		else {
			render_x = player_mobj->x;
			render_y = player_mobj->y;
			render_angle = player_mobj->angle;
		}

		original_x = player_mobj->x;
		original_y = player_mobj->y;
		original_angle = player_mobj->angle;

		player_mobj->x = render_x;
		player_mobj->y = render_y;
		player_mobj->angle = render_angle;

		byte r_color = 0, g_color = 0, b_color = 0;

		if (i == 0) { // Green
			g_color = flash;
		}
		else if (i == 1) { // Red
			r_color = flash;
		}
		else if (i == 2) { // Aqua
			g_color = flash; b_color = flash;
		}
		else if (i == 3) { // Blue
			b_color = flash;
		}
		// Default is (0,0,0) if i > 3, which is fine.
		AM_DrawTriangle(player_mobj, scale, amModeCycle, r_color, g_color, b_color);

		player_mobj->x = original_x;
		player_mobj->y = original_y;
		player_mobj->angle = original_angle;
	}
}

//
// AM_drawThings
//

static void AM_drawThings(void) {
	int     i;
	mobj_t* t;

	for (i = 0; i < numsectors; i++) {
		t = sectors[i].thinglist;

		while (t) {
			fixed_t original_t_x, original_t_y;
			angle_t original_t_angle;
			fixed_t render_t_x, render_t_y;
			angle_t render_t_angle;

			if (i_interpolateframes.value) {
				render_t_x = R_Interpolate(t->x, t->frame_x, true);
				render_t_y = R_Interpolate(t->y, t->frame_y, true);
				render_t_angle = R_Interpolate(t->angle, t->angle, true);
			}
			else {
				render_t_x = t->x;
				render_t_y = t->y;
				render_t_angle = t->angle;
			}

			original_t_x = t->x;
			original_t_y = t->y;
			original_t_angle = t->angle;

			t->x = render_t_x;
			t->y = render_t_y;
			t->angle = render_t_angle;

			//
			// draw thing triangles for automap cheat
			//
			if (amCheating == 2) {
				if (t->type != MT_PLAYER && am_drawobjects.value != 1) {
					//
					// shootable stuff are marked as red while normal things are blue
					//
					if (t->flags & MF_SHOOTABLE || t->flags & MF_MISSILE) {
						AM_DrawTriangle(t, scale, amModeCycle, 164, 0, 0);
					}
					else {
						AM_DrawTriangle(t, scale, amModeCycle, 51, 115, 179);
					}
				}

				if (am_drawobjects.value) {
					AM_DrawSprite(t, scale);
				}
			}
			//
			// draw colored keys and artifacts in automap for new players
			//
			else if (am_showkeymarkers.value) {
				if (t->type >= MT_ITEM_BLUECARDKEY && t->type <= MT_ITEM_ARTIFACT3) {
					byte r, g, b;

					switch (t->type) {
					case MT_ITEM_BLUECARDKEY:
					case MT_ITEM_BLUESKULLKEY:
						r = 0;
						g = 64;
						b = 255;
						break;

					case MT_ITEM_REDCARDKEY:
					case MT_ITEM_REDSKULLKEY:
						r = 255;
						g = 0;
						b = 0;
						break;

					case MT_ITEM_YELLOWCARDKEY:
					case MT_ITEM_YELLOWSKULLKEY:
						r = 255;
						g = 128;
						b = 0;
						break;

					case MT_ITEM_ARTIFACT1:
						r = 224;
						g = 56;
						b = 0;
						break;

					case MT_ITEM_ARTIFACT2:
						r = 0;
						g = 200;
						b = 224;
						break;

					case MT_ITEM_ARTIFACT3:
						r = 120;
						g = 0;
						b = 224;
						break;
					default:
						r = g = b = 255;
						break;
					}

					if (am_drawobjects.value != 1) {
						AM_DrawTriangle(t, scale, amModeCycle, r, g, b);
					}

					if (am_drawobjects.value) {
						AM_DrawSprite(t, scale);
					}
				}
			}

			t->x = original_t_x;
			t->y = original_t_y;
			t->angle = original_t_angle;

			t = t->snext;
		}
	}
}

//
// AM_Drawer
//

void AM_Drawer(void) {
	fixed_t base_x, base_y;
	angle_t base_angle;
	fixed_t render_x, render_y;
	angle_t view_for_draw;

	if (!automapactive) {
		return;
	}

	if (followplayer && !(am_flags & (AF_PANMODE | AF_PANGAMEPAD))) {
		if (i_interpolateframes.value) {
			base_x = R_Interpolate(plr->mo->x, automap_prev_x_interpolation, true);
			base_y = R_Interpolate(plr->mo->y, automap_prev_y_interpolation, true);
			base_angle = R_Interpolate(plr->mo->angle, automap_prev_ang_interpolation, true);
		}
		else {
			base_x = plr->mo->x;
			base_y = plr->mo->y;
			base_angle = plr->mo->angle;
		}
	}
	else {
		base_x = automapx;
		base_y = automapy;
		base_angle = automapangle;
	}
	render_x = base_x;
	render_y = base_y;

	if (plr->onground) {
		render_x += (quakeviewx >> 7);
		render_y += quakeviewy;
	}
	view_for_draw = base_angle - ANG90;

	AM_BeginDraw(view_for_draw, render_x, render_y);

	if (!amModeCycle) {
		AM_DrawMapped();
	}
	else {
		if (am_lines.value) {
			AM_DrawWalls();
		}
	}

	if (am_nodes.value) {
		AM_DrawNodes();
	}

	AM_drawPlayers();

	if (amCheating == 2 || am_showkeymarkers.value) {
		AM_drawThings();
	}

	if (plr->artifacts) {
		int x = 280;

		if (plr->artifacts & (1 << ART_TRIPLE)) {
			Draw_Sprite2D(SPR_ART3, 0, 0, x, 255, 1.0f, 0, WHITEALPHA(0x80));
			x -= 40;
		}

		if (plr->artifacts & (1 << ART_DOUBLE)) {
			Draw_Sprite2D(SPR_ART2, 0, 0, x, 255, 1.0f, 0, WHITEALPHA(0x80));
			x -= 40;
		}

		if (plr->artifacts & (1 << ART_FAST)) {
			Draw_Sprite2D(SPR_ART1, 0, 0, x, 255, 1.0f, 0, WHITEALPHA(0x80));
			x -= 40;
		}
	}

	AM_EndDraw();
}

//
// AM_RegisterCvars
//

void AM_RegisterCvars(void) {
	CON_CvarRegister(&am_lines);
	CON_CvarRegister(&am_nodes);
	CON_CvarRegister(&am_ssect);
	CON_CvarRegister(&am_fulldraw);
	CON_CvarRegister(&am_showkeycolors);
	CON_CvarRegister(&am_showkeymarkers);
	CON_CvarRegister(&am_drawobjects);
	CON_CvarRegister(&am_overlay);

	G_AddCommand("automap", CMD_Automap, 0);
	G_AddCommand("+automap_in", CMD_AutomapSetFlag, AF_ZOOMIN);
	G_AddCommand("-automap_in", CMD_AutomapSetFlag, AF_ZOOMIN | PCKF_UP);
	G_AddCommand("+automap_out", CMD_AutomapSetFlag, AF_ZOOMOUT);
	G_AddCommand("-automap_out", CMD_AutomapSetFlag, AF_ZOOMOUT | PCKF_UP);
	G_AddCommand("+automap_left", CMD_AutomapSetFlag, AF_PANLEFT);
	G_AddCommand("-automap_left", CMD_AutomapSetFlag, AF_PANLEFT | PCKF_UP);
	G_AddCommand("+automap_right", CMD_AutomapSetFlag, AF_PANRIGHT);
	G_AddCommand("-automap_right", CMD_AutomapSetFlag, AF_PANRIGHT | PCKF_UP);
	G_AddCommand("+automap_up", CMD_AutomapSetFlag, AF_PANTOP);
	G_AddCommand("-automap_up", CMD_AutomapSetFlag, AF_PANTOP | PCKF_UP);
	G_AddCommand("+automap_down", CMD_AutomapSetFlag, AF_PANBOTTOM);
	G_AddCommand("-automap_down", CMD_AutomapSetFlag, AF_PANBOTTOM | PCKF_UP);
	G_AddCommand("+automap_freepan", CMD_AutomapSetFlag, AF_PANMODE);
	G_AddCommand("-automap_freepan", CMD_AutomapSetFlag, AF_PANMODE | PCKF_UP);
	G_AddCommand("automap_follow", CMD_AutomapFollow, 0);
}
