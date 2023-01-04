// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// Copyright(C) 2012 Samuel Villarreal
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

#include "types.h"
#include "common.h"
#include "script.h"
#include "macro.h"
#include "parse.h"

int myargc;
char **myargv;

//
// PrintUsage
//

static void PrintUsage(void)
{
    Com_Printf("\t\t***************************************");
    Com_Printf("\t\t** BASIC LINE ACTION MACRO COMPILER  **");
    Com_Printf("\t\t**    email: svkaiser@gmail.com      **");
    Com_Printf("\t\t***************************************");
    Com_Printf("\n");
    Com_Printf("Usage: <blam.exe> <scriptfile> <outfile> <option>\n");
    Com_Printf("Options:\n-d\t\tDecompile script lump");
    Com_Printf("-i\tSet directory to search for files to include\n\n");
}

//
// main
//

int main(int argc, char **argv)
{
    dboolean decompile;
    int i;

    myargc = argc;
    myargv = argv;

    PrintUsage();

    if(!myargv[1])
    {
        Com_Printf("No input specified...");
        return 0;
    }

    if(!myargv[2])
    {
        Com_Printf("No output specified...");
        return 0;
    }

    SC_Init();
    M_Init();

    memset(ps_userdirectory, 0, sizeof(*ps_userdirectory));

    decompile = false;
    i = 3;

    while(myargv[i])
    {
        if(!strcmp(myargv[i], "-d"))
        {
            decompile = true;
        }
        else if(!strcmp(myargv[i], "-v"))
        {
            verbose = true;
            debugfile = fopen("blamdebug.txt", "w");
        }
        else if(!strcmp(myargv[i], "-i"))
        {
            int tail;

            sprintf(ps_userdirectory, "%s", myargv[++i]);
            tail = strlen(ps_userdirectory)-1;

            if(ps_userdirectory[tail] != '\\')
            {
                ps_userdirectory[tail+1] = '\\';
            }
        }
        else
        {
            Com_Error("Unknown option: %s", myargv[i]);
        }

        i++;
    }

    if(!decompile)
    {
        SC_Open(myargv[1]);
        PS_ParseScript();
        SC_Close();

        M_WriteMacroLump();
    }
    else
    {
        M_DecompileMacro(myargv[1]);
    }

    return 1;
}

