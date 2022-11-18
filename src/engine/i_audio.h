// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
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

#ifndef __I_AUDIO_H__
#define __I_AUDIO_H__

#include <SDL2/SDL_mixer.h>

extern Mix_Chunk *chunk;
extern Mix_Music *music;

extern void I_InitMixer();
extern void I_ShutdownSound();
extern int I_SetMasterVolume(int volume);
extern int I_SetSoundVolume(int volume);
extern int I_SetMusicVolume(int volume);
extern dboolean I_PauseSound(dboolean is_paused);
extern dboolean I_ResumeSound(dboolean is_resumed);
extern dboolean I_StopSound(dboolean is_stopped);
extern void I_GetMaxChannels();
extern dboolean I_StartMusic(dboolean is_started);
extern void I_LoadSF2();
extern dboolean I_StopMusic(dboolean is_stopped);

#endif // __I_AUDIO_H__
