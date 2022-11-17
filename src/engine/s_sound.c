// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 1993-1997 Id Software, Inc.
// Copyright(C) 2007-2012 Samuel Villarreal
// Copyright(C) 2022 André Guilherme
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
//-----------------------------------------------------------------------------
//
// DESCRIPTION: In-game Sound behavior
//
//-----------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>

#include "i_system.h"
#include "sounds.h"
#include "s_sound.h"
#include "z_zone.h"
#include "m_fixed.h"
#include "m_random.h"
#include "w_wad.h"
#include "doomdef.h"
#include "p_local.h"
#include "doomstat.h"
#include "tables.h"
#include "r_local.h"
#include "m_misc.h"
#include "p_setup.h"
#include "i_audio.h"
#include "con_console.h"

//
// Internals.
//
int S_AdjustSoundParams(fixed_t x, fixed_t y, int* vol, int* sep);

//
// S_Init
//
// Initializes sound stuff, including volume
// and the sequencer
//

void S_Init(void) {
   I_InitMixer();
   I_LoadSF2();
}

//
// S_SetSoundVolume
//

void S_SetSoundVolume(int volume) {
   I_SetSoundVolume(volume);  
}

//
// S_SetMusicVolume
//

void S_SetMusicVolume(int volume) {
   I_SetMusicVolume(volume);
}

//
// S_SetMasterOutput
//

void S_SetMasterVolume(int volume) {
     I_SetMasterVolume(volume);
}

//
// S_StartMusic
//

void S_StartMusic(int mnum) {
    I_StartMusic(mnum);
}

//
// S_StopMusic
//

void S_StopMusic(void) {

}

//
// S_ResetSound
//

void S_ResetSound(void) {

}

//
// S_PauseSound
//

void S_PauseSound(void) {
   I_PauseSound(true);
}

//
// S_ResumeSound
//

void S_ResumeSound(void) {
    I_ResumeSound(true);
}

//
// S_StopSound
//

void S_StopSound(mobj_t* origin, int sfx_id) {
  I_StopSound(true);
}

//
// S_GetActiveSounds
//

int S_GetActiveSounds(void) {

}

//
// S_RemoveOrigin
//

void S_RemoveOrigin(mobj_t* origin) {

}

//
// S_UpdateSounds
//

void S_UpdateSounds(void) {

}

//
// S_StartSound
//

void S_StartSound(mobj_t* origin, int sfx_id) {

}

//
// S_AdjustSoundParams
//
// Changes volume, stereo-separation, and pitch variables
// from the norm of a sound effect to be played.
// If the sound is not audible, returns a 0.
// Otherwise, modifies parameters and returns 1.
//

int S_AdjustSoundParams(fixed_t x, fixed_t y, int* vol, int* sep) {

}

//
// S_RegisterCvars
//

CVAR_EXTERNAL(s_soundfont);
CVAR_EXTERNAL(s_driver);

void S_RegisterCvars(void) {
	CON_CvarRegister(&s_soundfont);
}
