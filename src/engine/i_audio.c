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
#include "fluidsynth.h"

#include "doomtype.h"
#include "doomdef.h"
#include "i_system.h"
#include "i_audio.h"
#include "w_wad.h"
#include "z_zone.h"
#include "i_swap.h"
#include "con_console.h"    // for cvars

CVAR(s_soundfont, doomsnd.sf2);
