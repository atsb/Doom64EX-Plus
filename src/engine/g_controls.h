// Emacs style mode select   -*- C -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 1999-2000 Paul Brook
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

#ifndef G_CONTROLS_H
#define G_CONTROLS_H

// villsa 01052014 - changed to 420
#define NUMKEYS         420
#define NUMGAMEPADBTNS	60

#define PCKF_DOUBLEUSE  0x4000
#define PCKF_UP         0x8000
#define PCKF_COUNTMASK  0x00ff

typedef enum {
	PCKEY_ATTACK,
	PCKEY_USE,
	PCKEY_STRAFE,
	PCKEY_FORWARD,
	PCKEY_BACK,
	PCKEY_LEFT,
	PCKEY_RIGHT,
	PCKEY_STRAFELEFT,
	PCKEY_STRAFERIGHT,
	PCKEY_RUN,
	PCKEY_LOOKUP,
	PCKEY_LOOKDOWN,
	PCKEY_CENTER,
	PCKEY_QUICKSAVE,
	PCKEY_QUICKLOAD,
	PCKEY_SAVE,
	PCKEY_LOAD,
	PCKEY_SCREENSHOT,
	PCKEY_GAMMA,
	NUM_PCKEYS
} pckeys_t;

typedef struct {
	int            mousex;
	int            mousey;
	float		   joymovex;
	float		   joymovey;
	float		   joylookx;
	float		   joylooky;
	int            key[NUM_PCKEYS];
	int            nextweapon;
	int            sdclicktime;
	int            fdclicktime;
	int            flags;
} playercontrols_t;

#define PCF_NEXTWEAPON    0x01
#define PCF_FDCLICK        0x02
#define PCF_FDCLICK2    0x04
#define PCF_SDCLICK        0x08
#define PCF_SDCLICK2    0x10
#define PCF_PREVWEAPON    0x20
#define PCF_GAMEPAD     0x40

extern playercontrols_t    Controls;
extern char* G_GetConfigFileName(void);

#endif
