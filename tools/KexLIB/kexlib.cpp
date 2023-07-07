// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// Copyright(C) 2007-2014 Samuel Villarreal
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
// DESCRIPTION: Kex Library
//
//-----------------------------------------------------------------------------

#include "kexlib.hpp"

kexCvar kexlib::cvarDeveloper("developer", CVF_BOOL|CVF_CONFIG, "0", "Developer mode");
kexCvar kexlib::cvarBasePath("basepath", CVF_STRING|CVF_CONFIG, "", "Base file path to look for files");

void kexlib::Init(void) {
    kexlib::system->Init();
    kexlib::cvars->Init();
    kexObject::Init();
    kexlib::inputSystem->Init();
    kexlib::inputBinds->Init();
    kexlib::fileSystem->Init();

    kexlib::system->Printf("KexLib Initialized\n");
}
