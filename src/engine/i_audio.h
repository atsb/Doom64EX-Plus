// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 2007-2012 Samuel Villarreal
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

#ifndef __I_AUDIO_H__
#define __I_AUDIO_H__

#include <fmod.h>
#include <fmod_common.h>
#include <fmod_errors.h>
#include "m_fixed.h"

// 20120107 bkw: Linux users can change the default FluidSynth backend here:
#ifndef _WIN32
#define DEFAULT_FLUID_DRIVER "sndio"

// 20120203 villsa: add default for windows
#define DEFAULT_FLUID_DRIVER "dsound"
#elif __linux__
#define DEFAULT_FLUID_DRIVER "alsa"
#elif __APPLE__
#define DEFAULT_FLUID_DRIVER "coreaudio"
#else
#define DEFAULT_FLUID_DRIVER "sndio"
#endif

typedef struct {
    fixed_t x;
    fixed_t y;
    fixed_t z;
} sndsrc_t;


// FMOD Studio

#define MAX_GAME_SFX 256
#define MAX_FMOD_MUSIC_TRACKS 138

struct Sound {
    FMOD_SYSTEM* fmod_studio_system;
    FMOD_SYSTEM* fmod_studio_system_music;

    FMOD_SOUND* fmod_studio_sound[MAX_GAME_SFX];
    //FMOD_SOUND* fmod_studio_sound_plasma[MAX_GAME_SFX];
    FMOD_SOUND* fmod_studio_music[MAX_FMOD_MUSIC_TRACKS];

    FMOD_CHANNEL* fmod_studio_channel;
    FMOD_CHANNEL* fmod_studio_channel_music;
    FMOD_CHANNEL* fmod_studio_channel_loop;
    FMOD_CHANNEL* fmod_studio_channel_plasma_loop;

    FMOD_RESULT     fmod_studio_result;

    FMOD_CHANNELGROUP* master;
    FMOD_CHANNELGROUP* master_music;

    FMOD_VECTOR vec_position;
    FMOD_VECTOR vec_velocity;
    FMOD_VECTOR vec_front;
    FMOD_VECTOR vec_up;

    FMOD_CREATESOUNDEXINFO  extinfo;
};

struct Reverb {
    FMOD_REVERB3D* fmod_reverb;
};

int I_GetMaxChannels(void);
int I_GetVoiceCount(void);
sndsrc_t* I_GetSoundSource(int c);

void Chan_SetMusicVolume(float volume);
void Chan_SetSoundVolume(float volume);
void Seq_SetGain(float db);

void FMOD_CreateMusicTracksInit(void);
void FMOD_CreateSfxTracksInit(void);
int FMOD_StartSound(int sfx_id, sndsrc_t* origin, int volume, int pan);
int FMOD_StartSoundPlasma(int sfx_id);
void FMOD_StopSound(sndsrc_t* origin, int sfx_id);
int FMOD_StartMusic(int mus_id);
void FMOD_StopMusic(sndsrc_t* origin, int mus_id);
int FMOD_StartSFXLoop(int sfx_id, int volume);
int FMOD_StopSFXLoop(void);
int FMOD_StartPlasmaLoop(int sfx_id, int volume);
void FMOD_StopPlasmaLoop(void);
void FMOD_PauseMusic(void);
void FMOD_ResumeMusic(void);
void FMOD_PauseSFXLoop(void);
void FMOD_ResumeSFXLoop(void);

void I_InitSequencer(void);
void I_ShutdownSound(void);
void I_UpdateChannel(int c, int volume, int pan, fixed_t x, fixed_t y);
void I_RemoveSoundSource(int c);
void I_SetMusicVolume(float volume);
void I_SetSoundVolume(float volume);
void I_ResetSound(void);
void I_PauseSound(void);
void I_ResumeSound(void);
void I_SetGain(float db);
void I_StopSound(sndsrc_t* origin, int sfx_id);
void I_StartMusic(int mus_id);

// FMOD Studio Listener Update
void I_UpdateListenerPosition(fixed_t player_world_x, fixed_t player_world_y_depth, fixed_t player_eye_world_z_height, angle_t view_angle);

#endif // __I_AUDIO_H__
