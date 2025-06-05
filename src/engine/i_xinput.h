// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 2014 Zohar Malamant
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

#ifndef __I_JOYSTICK__
#define __I_JOYSTICK__

#include <SDL.h>

void I_InitJoystick(void);
char I_JoystickEvent(const SDL_Event* Event);

void I_RegisterJoystickCvars(void);

void I_ReadJoystick(void);

const char* I_JoystickName(void);

int I_JoystickToKey(int key);

#endif /* __I_JOYSTICK__ */
