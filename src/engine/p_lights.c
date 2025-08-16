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
// DESCRIPTION:
//    Handle Sector base lighting effects.
//
//-----------------------------------------------------------------------------

#include "z_zone.h"
#include "m_random.h"
#include "doomstat.h"
#include "p_local.h"
#include "r_lights.h"
#include "con_cvar.h"

CVAR_EXTERNAL(v_accessibility);

//------------------------------------------------------------------------
//
// FIRELIGHT FLICKER
//
//------------------------------------------------------------------------

//
// T_FireFlicker
//

void T_FireFlicker(fireflicker_t* flick) {
	int    amount;

	if (--flick->count) {
		return;
	}

	if (flick->sector->special != flick->special)
	{
		flick->sector->lightlevel = 0;
		P_RemoveThinker(&flick->thinker);
		return;
	}

	amount = (P_Random() & 31);
	flick->sector->lightlevel = amount;
	flick->count = 3;
}

//
// P_SpawnFireFlicker
//

void P_SpawnFireFlicker(sector_t* sector) {
	fireflicker_t* flick;

	flick = Z_Malloc(sizeof(*flick), PU_LEVSPEC, 0);

	P_AddThinker(&flick->thinker);

	flick->thinker.function.acp1 = (actionf_p1)T_FireFlicker;
	flick->sector = sector;
	flick->special = sector->special;
	flick->count = 3;
}

//------------------------------------------------------------------------
//
// BROKEN LIGHT FLASHING
//
//------------------------------------------------------------------------

//
// T_LightFlash
// Do flashing lights.
//

void T_LightFlash(lightflash_t* flash) {
	if (v_accessibility.value < 1)
	{
		if (--flash->count) {
			return;
		}

		if (flash->special != flash->sector->special) {
			flash->sector->lightlevel = 0;
			P_RemoveThinker(&flash->thinker);
			return;
		}

		if (flash->sector->lightlevel == 32) {
			flash->sector->lightlevel = 0;
			flash->count = (P_Random() & 7) + 1;
		}
		else {
			flash->sector->lightlevel = 32;
			flash->count = (P_Random() & 32) + 1;
		}
	}
}

//
// P_SpawnLightFlash
// After the map has been loaded, scan each sector
// for specials that spawn thinkers
//
void P_SpawnLightFlash(sector_t* sector) {
	if (v_accessibility.value < 1)
	{
		lightflash_t* flash;

		flash = Z_Malloc(sizeof(*flash), PU_LEVSPEC, 0);

		P_AddThinker(&flash->thinker);

		flash->thinker.function.acp1 = (actionf_p1)T_LightFlash;
		flash->sector = sector;
		flash->special = sector->special;
		flash->count = (P_Random() & 63) + 1;
	}
}

//------------------------------------------------------------------------
//
// STROBE LIGHT FLASHING
//
//------------------------------------------------------------------------

//
// T_StrobeFlash
//

void T_StrobeFlash(strobe_t* flash) {
	if (v_accessibility.value < 1)
	{
		if (--flash->count)
		{
			return;
		}

		if (flash->sector->special != flash->special)
		{
			flash->sector->lightlevel = 0;
			P_RemoveThinker(&flash->thinker);
			return;
		}

		if (flash->sector->lightlevel == 0)
		{
			flash->sector->lightlevel = flash->maxlight;
			flash->count = flash->brighttime;
		}
		else
		{
			flash->sector->lightlevel = 0;
			flash->count = flash->darktime;
		}
	}
}

//
// P_SpawnStrobeFlash
// After the map has been loaded, scan each sector
// for specials that spawn thinkers
//

void P_SpawnStrobeFlash(sector_t* sector, int speed) {
	if (v_accessibility.value < 1)
	{
		strobe_t* flash;

		flash = Z_Malloc(sizeof(*flash), PU_LEVSPEC, 0);
		P_AddThinker(&flash->thinker);
		flash->thinker.function.acp1 = (actionf_p1)T_StrobeFlash;
		flash->sector = sector;
		flash->special = sector->special;
		flash->brighttime = STROBEBRIGHT;
		flash->maxlight = 16;
		flash->darktime = speed;
		flash->count = (P_Random() & 7) + 1;
	}
}

//
// P_SpawnStrobeAltFlash
// Alternate variation of P_SpawnStrobeFlash
//

void P_SpawnStrobeAltFlash(sector_t* sector, int speed) {      // 0x80015C44
	if (v_accessibility.value < 1)
	{
		strobe_t* flash;

		flash = Z_Malloc(sizeof(*flash), PU_LEVSPEC, 0);
		P_AddThinker(&flash->thinker);
		flash->thinker.function.acp1 = (actionf_p1)T_StrobeFlash;
		flash->sector = sector;
		flash->special = sector->special;
		flash->brighttime = STROBEBRIGHT2;
		flash->maxlight = 127;
		flash->darktime = speed;
		flash->count = 1;
	}
}

//
// EV_StartLightStrobing
// Start strobing lights (usually from a trigger)
//

void EV_StartLightStrobing(line_t* line) {
	if (v_accessibility.value < 1)
	{
		int            secnum;
		sector_t* sec;

		secnum = -1;
		while ((secnum = P_FindSectorFromLineTag(line, secnum)) >= 0) {
			sec = &sectors[secnum];
			if (sec->specialdata) {
				continue;
			}
			P_SpawnStrobeFlash(sec, SLOWDARK);
		}
	}
}

//------------------------------------------------------------------------
//
// GLOWING LIGHTS
//
//------------------------------------------------------------------------

//
// T_Glow
//

void T_Glow(glow_t* g) {
	if (v_accessibility.value < 1)
	{
		if (--g->count) {
			return;
		}

		if (g->sector->special != g->special)
		{
			g->sector->lightlevel = 0;
			P_RemoveThinker(&g->thinker);
			return;
		}

		g->count = 2;

		switch (g->direction)
		{
		case -1:		/* DOWN */
			g->sector->lightlevel -= GLOWSPEED;
			if (g->sector->lightlevel < g->minlight)
			{
				g->sector->lightlevel = g->minlight;

				if (g->type == PULSERANDOM)
					g->maxlight = (P_Random() & 31) + 17;

				g->direction = 1;
			}
			break;
		case 1:			/* UP */
			g->sector->lightlevel += GLOWSPEED;
			if (g->maxlight < g->sector->lightlevel)
			{
				g->sector->lightlevel = g->maxlight;

				if (g->type == PULSERANDOM)
					g->minlight = (P_Random() & 15);

				g->direction = -1;
			}
			break;
		}
	}
}

//
// P_SpawnGlowingLight
//

void P_SpawnGlowingLight(sector_t* sector, byte type) {
	if (v_accessibility.value < 1)
	{
		glow_t* g;

		g = Z_Malloc(sizeof(*g), PU_LEVSPEC, 0);

		P_AddThinker(&g->thinker);
		g->count = 2;
		g->direction = 1;
		g->sector = sector;
		g->type = type;
		g->special = sector->special;
		g->thinker.function.acp1 = (actionf_p1)T_Glow;
		g->minlight = 0;

		if (type == PULSENORMAL)
		{
			g->maxlight = 32;
		}
		else if (type == PULSERANDOM || type == PULSESLOW)
		{
			g->maxlight = 48;
		}
	}
}

//------------------------------------------------------------------------
//
// SEQUENCED LIGHTS
//
//------------------------------------------------------------------------

#define SEQUENCELIGHTMAX    48

//
// T_Sequence
//

void T_Sequence(sequenceGlow_t* seq) {
	sector_t* sector;
	int i;

	if (--seq->count) {
		return;
	}

	sector = seq->sector;

	if (seq->sector->special != seq->special)
	{
		seq->sector->lightlevel = 0;
		P_RemoveThinker(&seq->thinker);
		return;
	}

	seq->count = 1;

	switch (seq->start)
	{
	case -1:		/* DOWN */
		seq->sector->lightlevel -= GLOWSPEED;

		if (seq->sector->lightlevel > 0)
			return;

		seq->sector->lightlevel = 0;

		if (seq->headsector == NULL)
		{
			seq->sector->special = 0;
			P_RemoveThinker(&seq->thinker);
			return;
		}

		seq->start = 0;
		break;
	case 0:		    /* CHECK */
		if (!seq->headsector->lightlevel)
			return;

		seq->start = 1;
		break;
	case 1:			/* UP */
		seq->sector->lightlevel += GLOWSPEED;
		if (seq->sector->lightlevel < (SEQUENCELIGHTMAX + 1))
		{
			if (seq->sector->lightlevel != 8)
				return;

			if (seq->sector->linecount <= 0)
				return;

			for (i = 0; i < seq->sector->linecount; i++)
			{
				sector = seq->sector->lines[i]->backsector;

				if (sector && (sector->special == 0))
				{
					if (sector->tag == (seq->sector->tag + 1))
					{
						sector->special = seq->sector->special;
						P_SpawnSequenceLight(sector, false);
					}
				}
			}
		}
		else
		{
			seq->sector->lightlevel = SEQUENCELIGHTMAX;
			seq->start = -1;
		}
		break;
	}
}

//
// P_SpawnSequenceLight
//

void P_SpawnSequenceLight(sector_t* sector, boolean first) {
	sequenceGlow_t* seq;
	sector_t* headsector = NULL;
	int i = 0;

	if (!sector->linecount) {
		return;
	}

	if (first) {
		for (i = 0; i < sector->linecount; i++) {
			headsector = sector->lines[i]->frontsector;
			if (!headsector) {  // this should never happen
				return;
			}

			if (headsector == sector) {
				continue;
			}

			if (headsector->tag == sector->tag) {
				break;
			}
		}
	}

	seq = Z_Malloc(sizeof(*seq), PU_LEVSPEC, 0);
	P_AddThinker(&seq->thinker);
	seq->thinker.function.acp1 = (actionf_p1)T_Sequence;
	seq->sector = sector;
	seq->special = sector->special;
	seq->count = 1;
	seq->index = sector->tag;
	seq->start = (headsector == NULL ? 1 : 0);
	seq->headsector = headsector;
}

//------------------------------------------------------------------------
//
// COMBINED LIGHT THINKERS
//
//------------------------------------------------------------------------

//
// T_Combine
//

void T_Combine(combine_t* combine) {
	sector_t* sector;

	sector = combine->sector;

	if (combine->special != sector->special ||
		combine->func != combine->combiner->function.acp1) {
		sector->lightlevel = 0;
		P_RemoveThinker(&combine->thinker);
		return;
	}

	sector->lightlevel = ((combine_t*)combine->combiner)->sector->lightlevel;
}

//
// P_CombineLightSpecials
//

void P_CombineLightSpecials(sector_t* sector) {
	actionf_p1 func;
	thinker_t* thinker;
	combine_t* combine;

	switch (sector->special) {
	case 1:
		func = (actionf_p1)T_LightFlash;
		break;
	case 2:
	case 3:
	case 202:
	case 203:
	case 204:
	case 205:
	case 206:
	case 208:
		func = (actionf_p1)T_StrobeFlash;
		break;
	case 8:
	case 9:
	case 11:
		func = (actionf_p1)T_Glow;
		break;
	case 17:
		func = (actionf_p1)T_FireFlicker;
		break;
	default:
		return;
	}

	for (thinker = thinkercap.next; thinker != &thinkercap; thinker = thinker->next) {
		if ((actionf_p1)func != (actionf_p1)thinker->function.acp1) {
			continue;
		}

		combine = Z_Malloc(sizeof(*combine), PU_LEVSPEC, 0);

		P_AddThinker(&combine->thinker);

		combine->sector = sector;
		combine->special = sector->special;
		combine->combiner = thinker;
		combine->func = func;
		combine->thinker.function.acp1 = (actionf_p1)T_Combine;
		return;
	}
}

//------------------------------------------------------------------------
//
// INTERPOLATED LIGHT SPECIALS
//
//------------------------------------------------------------------------

int P_FindSectorFromTag(int tag);

//
// T_LightMorph
//
// Interpolates one light_t's RGB to another light_t's RGB
//

void T_LightMorph(lightmorph_t* lt) {
	lt->inc += 4;

	if (lt->inc > 256) {
		lt->dest->base_r = lt->dest->active_r;
		lt->dest->base_g = lt->dest->active_g;
		lt->dest->base_b = lt->dest->active_b;

		P_RemoveThinker(&lt->thinker);
		return;
	}

	lt->dest->active_r = (lt->r + ((lt->inc * (lt->src->base_r - lt->r)) >> 8));
	lt->dest->active_g = (lt->g + ((lt->inc * (lt->src->base_g - lt->g)) >> 8));
	lt->dest->active_b = (lt->b + ((lt->inc * (lt->src->base_b - lt->b)) >> 8));
}

//
// P_UpdateLightThinker
//

void P_UpdateLightThinker(light_t* destlight, light_t* srclight) {
	lightmorph_t* lt;

	lt = Z_Malloc(sizeof(*lt), PU_LEVSPEC, 0);
	P_AddThinker(&lt->thinker);
	lt->thinker.function.acp1 = (actionf_p1)T_LightMorph;

	if (destlight) {
		destlight->r = srclight->r;
		destlight->g = srclight->g;
		destlight->b = srclight->b;
	}

	lt->inc = 0;
	lt->src = srclight; // the light to morph to
	lt->dest = destlight == NULL ? srclight : destlight; // the light to morph from
	lt->r = destlight == NULL ? 0 : destlight->base_r;
	lt->g = destlight == NULL ? 0 : destlight->base_g;
	lt->b = destlight == NULL ? 0 : destlight->base_b;
}

//
// P_DoSectorLightChange
//

int P_DoSectorLightChange(line_t* line, short tag) {
	int j = 0;
	int ptr1 = 0;
	int ptr2 = 0;
	sector_t* sec1;
	sector_t* sec2;
	int secnum;

	secnum = P_FindSectorFromTag(tag);

	if (secnum == -1) {
		return 0;
	}

	sec2 = &sectors[secnum];

	secnum = -1;

	while ((secnum = P_FindSectorFromLineTag(line, secnum)) >= 0) {
		sec1 = &sectors[secnum];

		for (j = 0; j < 5; j++) {
			ptr1 = (sec1->colors[j]);
			ptr2 = (sec2->colors[j]);
			P_UpdateLightThinker(&lights[ptr1], &lights[ptr2]);
		}
	}

	return 1;
}

//
// P_ChangeLightByTag
//
// This has to be early stuff because this is only
// used once in the entire game and the only place where
// internal light tags are used..
//

boolean P_ChangeLightByTag(int tag1, int tag2) {
	light_t* l1 = NULL;
	light_t* l2 = NULL;
	int i = 0;
	boolean ok = false;

	for (i = 256; i < numlights + 1; i++) {
		if (i >= numlights) {
			return false;
		}

		l1 = &lights[i];
		if (l1->tag == tag1) {
			break;
		}
	}

	if (l1 == NULL) {
		return ok;
	}

	for (i = 256; i < numlights; i++) {
		l2 = &lights[i];

		if (l2->tag == tag2) {
			ok = true;
			P_UpdateLightThinker(l1, l2);
		}
	}

	return ok;
}

//------------------------------------------------------------------------
//
// FADE IN BRIGHTNESS EFFECT
//
//------------------------------------------------------------------------

typedef struct {
	thinker_t thinker;
	float factor;
} fadebright_t;

CVAR_EXTERNAL(i_brightness);

//
// T_FadeInBrightness
//

void T_FadeInBrightness(fadebright_t* fb) {
	fb->factor += 2;
	if (fb->factor < (i_brightness.value + 100)) {
		R_SetLightFactor(fb->factor);
	}
	else {
		P_RemoveThinker(&fb->thinker);
	}
}

//
// P_FadeInBrightness
//

void P_FadeInBrightness(void) {
	fadebright_t* fb;

	fb = Z_Malloc(sizeof(*fb), PU_LEVSPEC, 0);
	P_AddThinker(&fb->thinker);
	fb->thinker.function.acp1 = (actionf_p1)T_FadeInBrightness;
	fb->factor = 0;
}
