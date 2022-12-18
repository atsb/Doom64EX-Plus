// Emacs style mode select   -*- C -*-
//-----------------------------------------------------------------------------
//
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
// DESCRIPTION:
//		It stores some OpenGL utils here:
// 
//-----------------------------------------------------------------------------

#include "gl_utils.h"

void GL_GetVersion(int major, int minor)
{
	OGL_WINDOW_HINT(OGL_MAJOR_VERSION, major);
	OGL_WINDOW_HINT(OGL_MINOR_VERSION, minor);
}

void GL_DestroyWindow(OGL_DEFS)
{
	if(Window)
	{
		OGL_DESTROY_WINDOW(Window);
		Window = NULL;
	}
}