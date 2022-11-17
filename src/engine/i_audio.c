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
//
// DESCRIPTION: Low-level audio API. Incorporates a sequencer system to
//              handle all sounds and music. All code related to the sequencer
//              is kept in it's own module and seperated from the rest of the
//              game code.
//
//-----------------------------------------------------------------------------

#include <stdlib.h>
#include <stdio.h>

#ifndef _WIN32
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#include <SDL2/SDL.h>
#include "doomtype.h"
#include "doomdef.h"
#include "i_system.h"
#include "i_audio.h"
#include "w_wad.h"
#include "z_zone.h"
#include "i_swap.h"
#include "con_console.h"    // for cvars

CVAR(s_soundfont, doomsnd.sf2);

Mix_Chunk *chunk;
Mix_Music *music;

void I_LoadSF2()
{
  dboolean sf2 = false;
  char *filename = I_FindDataFile("doomsnd.sf2");
  if(!sf2)
  {
    I_Printf("Failed to load: %s", filename);
  }
  else
  {
     music = Mix_LoadMUS(filename);
  }
}

void I_InitMixer()
{
   if(Mix_Init(MIX_INIT_MID) < 0)
   {
     printf("Failed to initialize SDL2_mixer");
   } 
}

void I_ShutdownSound()
{
  Mix_Quit();
  Mix_HaltMusic();
  Mix_CloseAudio(); 
  SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

void I_GetMaxChannels()
{
    Mix_AllocateChannels(MIX_CHANNELS);
}

dboolean I_StartMusic(dboolean is_started)
{
    Mix_music handlemus; //TODO: = mus_id;
    const char *name;
    if(!start =! name)
    {
       I_printf("Failed to load the music, name: %s", name);
       return is_started = false;
    }
    else
    {
      Mix_PlayMusic(handlemus, 0);
      name = Mix_GetMusicTitleTag(handlemus);
      I_printf("Playing the music name, %s", name);
      return is_started = true;
    }

    return is_started;
}

dboolean I_StopMusic(dboolean is_stopped)
{
  Mix_Music *free;
  if(is_stopped)
  {
     printf("failed to stop muscic");
     return is_stopped = false;
  }
  else
  {
    Mix_FreeMusic(free);
    return is_stopped = true;
  }

  return is_stopped;
}

int I_SetSoundVolume(int volume)
{
    int channel;
    Mix_AllocateChannels(MIX_CHANNELS);
    return Mix_Volume(-1, volume);
}

int I_SetMusicVolume(int volume)
{
  Mix_AllocateChannels(MIX_CHANNELS);
  return Mix_VolumeMusic(volume);
}

int I_SetMasterVolume(int volume)
{
    return Mix_MasterVolume(volume);
}

dboolean I_PauseSound(dboolean is_paused)
{
   if(is_paused == true)
   {
     Mix_Pause(-1); //Get the all channels
     return is_paused = true;
   }
   else if(is_paused == false)
   {
     printf("The channel is not paused");
     return is_paused = false;
   }
   else
   {
    printf("Failed to pause the channel(s), Error: s %s", Mix_GetError());
    return is_paused = false;
   }
  return is_paused;
}  

dboolean I_ResumeSound(dboolean is_resumed)
{
     if(is_resumed == true)
     {
        Mix_Resume(-1);
        return is_resumed = true;
     }
     else if(is_resumed == false)
     {
       printf("The channel is not resumed");
       return is_resumed = false;
     }
     else
     {
       printf("Failed to resume the channel(s), Error: %s", Mix_GetError());-
       return is_resumed = false;
     }
    return is_resumed;
}

dboolean I_StopSound(dboolean is_stopped)
{
   if(is_stopped == true)
   {
     Mix_HaltChannel(-1);
     return is_stopped;
   }
   if(is_stopped == false)
   {
     printf("The channel(s) is not stopped");
     return is_stopped;
   }
   else 
   {
     printf("Failed to stop the channel, Error: %s", Mix_GetError());
   }
   return is_stopped;
}
