// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// Copyright(C) 2013 Samuel Villarreal
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

#ifndef __DEFINITION_H__
#define __DEFINITION_H__

class kexDefinition {
public:
    void                            Parse(kexLexer *lexer);

    filepath_t                      fileName;
    kexHashList<kexKeyMap>          entries;
};

class kexDefManager {
public:
    kexDefinition                   *LoadDefinition(const char *file);
    kexKeyMap                       *FindDefEntry(const char *name);
                                    
    kexHashList<kexDefinition>      defs;
};

#endif
