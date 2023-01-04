// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// Copyright(C) 2014 Samuel Villarreal
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
// DESCRIPTION: Definition system
//
//-----------------------------------------------------------------------------

#include "kexlib.h"

kexDefManager defManager;
kexDefManager *kexlib::defs = &defManager;

//
// kexDefinition::Parse
//

void kexDefinition::Parse(kexLexer *lexer) {
    kexKeyMap *defEntry;
    kexStr key;
    kexStr val;

    while(lexer->CheckState()) {
        lexer->Find();

        switch(lexer->TokenType()) {
        case TK_EOF:
            return;
        case TK_IDENIFIER:
            defEntry = entries.Add(lexer->Token());

            lexer->ExpectNextToken(TK_LBRACK);
            while(1) {
                lexer->Find();
                if(lexer->TokenType() == TK_RBRACK || lexer->TokenType() == TK_EOF) {
                    break;
                }

                key = lexer->Token();

                lexer->Find();
                if(lexer->TokenType() == TK_RBRACK || lexer->TokenType() == TK_EOF) {
                    break;
                }

                val = lexer->Token();

                defEntry->Add(key.c_str(), val.c_str());
            }
            break;
        default:
            break;
        }
    }
}

//
// kexDefManager::LoadDefinition
//

kexDefinition *kexDefManager::LoadDefinition(const char *file) {
    kexDefinition *def = NULL;

    if(file == NULL || file[0] == 0) {
        return NULL;
    }

    if(!(def = defs.Find(file))) {
        kexLexer *lexer;

        if(strlen(file) >= MAX_FILEPATH) {
            kexlib::system->Error("kexDefManager::LoadDefinition: \"%s\" is too long", file);
        }

        if(!(lexer = kexlib::parser->Open(file))) {
            return NULL;
        }

        def = defs.Add(file);
        strncpy(def->fileName, file, MAX_FILEPATH);

        def->Parse(lexer);

        // we're done with the file
        kexlib::parser->Close();
    }

    return def;
}

//
// kexDefManager::FindDefEntry
//
// Retrives an entry in a definition file
// The name should be <path of def file>@<entry name>
// Example: defs/damage.def@MeleeBlunt
//

kexKeyMap *kexDefManager::FindDefEntry(const char *name) {
    kexDefinition *def;
    kexKeyMap *defEntry;
    filepath_t tStr;
    int pos;
    int len;

    pos = kexStr::IndexOf(name, "@");

    if(pos == -1) {
        return NULL;
    }

    len = strlen(name);
    strncpy(tStr, name, pos);
    tStr[pos] = 0;

    if((def = LoadDefinition(tStr))) {
        strncpy(tStr, name + pos + 1, len - pos);
        tStr[len - pos] = 0;

        if((defEntry = def->entries.Find(tStr))) {
            return defEntry;
        }

        kexlib::system->Warning("kexDefManager::FindDefEntry: %s not found\n", tStr);
    }

    return NULL;
}
