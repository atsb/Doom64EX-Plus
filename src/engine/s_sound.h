// Emacs style mode select   -*- C -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 1993-1997 Id Software, Inc.
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


#ifndef __S_SOUND__
#define __S_SOUND__

#include "p_mobj.h"

//
// Initializes sound stuff, including volume
// Sets channels, SFX and music volume,
//  allocates channel buffer
//
void S_Init(void);

void S_SetSoundVolume(float volume);
void S_SetMusicVolume(float volume);
void S_SetGainOutput(float db);

void S_ResetSound(void);
void S_PauseSound(void);
void S_ResumeSound(void);

//
// Start sound for thing at <origin>
//  using <sound_id> from sounds.h
//
void S_StartSound(mobj_t* origin, int sound_id);
void S_StartPlasmaSound(mobj_t* origin, int sound_id);
void S_StartPlasmaGunLoop(mobj_t* origin, int sfx_id, int volume);
void S_UpdateSounds(void);
void S_RemoveOrigin(mobj_t* origin);

// Will start a sound at a given volume.
void S_StartSoundAtVolume(mobj_t* origin, int sound_id, int volume);

// Stop sound for thing at <origin>
void S_StopSound(mobj_t* origin, int sfx_id);
void S_StartLoopingSound(mobj_t* origin, int sfx_id, int volume);

// Stop sound
void S_StopLoopingSound(void);
void S_StopPlasmaGunLoop(void);

int S_GetActiveSounds(void);

// Start music using <music_id> from sounds.h
void S_StartMusic(int mnum);
void S_StopMusic(void);

void S_RegisterCvars(void);


#endif
