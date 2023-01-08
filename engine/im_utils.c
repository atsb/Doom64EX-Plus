// Emacs style mode select   -*- C -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 2023 André Guilherme 
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

#include "im_utils.h"
ImGuiIO *io;
ImGuiContext* ctx;
//Only for code testing since ImGUI is C++ Aka: C++98 this wrapper is pure C.
void IM_Init()
{
	io = igGetIO();
	ctx = igCreateContext(NULL);
	igStyleColorsDark(NULL);

	igBegin("Test", NULL, 0);
	igText("Test");
	igButton("Test", (struct ImVec2) { 0, 0 });
	igEnd();
}