//
// Copyright(C) 2005-2014 Simon Howard
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
//
// Main dehacked code
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef C89
#include <stdbool.h>
#endif
#include "i_w3swrapper.h"
#include <ctype.h>

#include "i_opndir.h"
#include "doomtype.h"
#include "i_system.h"
#include "w_wad.h"
#include "w_file.h"
#include "m_misc.h"
#include "deh_defs.h"
#include "deh_io.h"

extern deh_section_t *deh_section_types[];
extern char* deh_signatures[];

static boolean deh_initialized = false;

void DEH_Checksum(sha1_digest_t digest)
{
    sha1_context_t sha1_context;
    unsigned int i;

    SHA1_Init(&sha1_context);

    for (i=0; deh_section_types[i] != NULL; ++i)
    {
        if (deh_section_types[i]->sha1_hash != NULL)
        {
            deh_section_types[i]->sha1_hash(&sha1_context);
        }
    }

    SHA1_Final(digest, &sha1_context);
}

// Called on startup to call the Init functions

static void InitializeSections(void)
{
    unsigned int i;

    for (i=0; deh_section_types[i] != NULL; ++i)
    {
        if (deh_section_types[i]->init != NULL)
        {
            deh_section_types[i]->init();
        }
    }
}

static void DEH_Init(void)
{
    // Call init functions for all the section definitions.
    InitializeSections();

    deh_initialized = true;
}

// Given a section name, get the section structure which corresponds

static deh_section_t *GetSectionByName(char *name)
{
    unsigned int i;

    // we explicitely do not recognize [STRINGS] sections at all

    if (!strncasecmp("[STRINGS]", name, 9))
    {
        return NULL;
    }

    for (i=0; deh_section_types[i] != NULL; ++i)
    {
        if (!strcasecmp(deh_section_types[i]->name, name))
        {
            return deh_section_types[i];
        }
    }

    return NULL;
}

// Is the string passed just whitespace?

static boolean IsWhitespace(char *s)
{
    for (; *s; ++s)
    {
        if (!isspace(*s))
            return false;
    }

    return true;
}

// Strip whitespace from the start and end of a string

static char *CleanString(char *s)
{
    char *strending;

    // Leading whitespace

    while (*s && isspace(*s))
        ++s;

    // Trailing whitespace
   
    strending = s + strlen(s) - 1;

    while (strlen(s) > 0 && isspace(*strending))
    {
        *strending = '\0';
        --strending;
    }

    return s;
}

// This pattern is used a lot of times in different sections, 
// an assignment is essentially just a statement of the form:
//
// Variable Name = Value
//
// The variable name can include spaces or any other characters.
// The string is split on the '=', essentially.
//
// Returns true if read correctly

boolean DEH_ParseAssignment(char *line, char **variable_name, char **value)
{
    char *p;

    // find the equals
    
    p = strchr(line, '=');

    if (p == NULL)
    {
        return false;
    }

    // variable name at the start
    // turn the '=' into a \0 to terminate the string here

    *p = '\0';
    *variable_name = CleanString(line);
    
    // value immediately follows the '='
    
    *value = CleanString(p+1);
    
    return true;
}

static boolean CheckSignatures(deh_context_t *context)
{
    size_t i;
    char *line;
    
    // Read the first line

    line = DEH_ReadLine(context, false);

    if (line == NULL)
    {
        return false;
    }

    // Check all signatures to see if one matches

    for (i = 0; deh_signatures[i] != NULL; ++i)
    {
        if (!strcmp(deh_signatures[i], line))
        {
            return true;
        }
    }

    return false;
}

// Parses a dehacked file by reading from the context

static void DEH_ParseContext(deh_context_t *context)
{
    deh_section_t *current_section = NULL;
    char section_name[20];
    void *tag = NULL;
    boolean extended;
    char *line;

    // Read the header and check it matches the signature

    if (!CheckSignatures(context))
    {
        DEH_Error(context, "This is not a valid dehacked patch file!");
    }

    // Read the file

    while (!DEH_HadError(context))
    {
        // Read the next line. We only allow the special extended parsing
        // for the BEX [STRINGS] section.
        extended = current_section != NULL
                && !strcasecmp(current_section->name, "[STRINGS]");
        line = DEH_ReadLine(context, extended);

        // end of file?

        if (line == NULL)
        {
            return;
        }

        while (line[0] != '\0' && isspace(line[0]))
            ++line;

        if (IsWhitespace(line))
        {
            if (current_section != NULL)
            {
                // end of section

                if (current_section->end != NULL)
                {
                    current_section->end(context, tag);
                }

                //printf("end %s tag\n", current_section->name);
                current_section = NULL;
            }
        }
        else
        {
            if (current_section != NULL)
            {
                // parse this line

                current_section->line_parser(context, line, tag);
            }
            else
            {
                // possibly the start of a new section

                sscanf(line, "%19s", section_name);

                current_section = GetSectionByName(section_name);

                if (current_section != NULL)
                {
                    tag = current_section->start(context, line);
                    //printf("started %s tag\n", section_name);
                }
                else
                {
                    //printf("unknown section name %s\n", section_name);
                }
            }
        }
    }
}

// Parses a dehacked file

int DEH_LoadFile(char *filename)
{
    deh_context_t *context;

    if (!deh_initialized)
    {
        DEH_Init();
    }

    printf(" loading %s\n", filename);

    context = DEH_OpenFile(filename);

    if (context == NULL)
    {
        fprintf(stderr, "DEH_LoadFile: Unable to open %s\n", filename);
        return 0;
    }

    DEH_ParseContext(context);

    DEH_CloseFile(context);

    if (DEH_HadError(context))
    {
        I_Error("Error parsing dehacked file");
    }

    return 1;
}

// Load dehacked file from WAD lump.
// If allow_long is set, allow long strings and cheats just for this lump.

int DEH_LoadLump(int lumpnum, boolean allow_long, boolean allow_error)
{
    deh_context_t *context;

    if (!deh_initialized)
    {
        DEH_Init();
    }

    context = DEH_OpenLump(lumpnum);

    if (context == NULL)
    {
        fprintf(stderr, "DEH_LoadFile: Unable to open lump %i\n", lumpnum);
        return 0;
    }

    DEH_ParseContext(context);

    DEH_CloseFile(context);

    // If there was an error while parsing, abort with an error, but allow
    // errors to just be ignored if allow_error=true.
    if (!allow_error && DEH_HadError(context))
    {
        I_Error("Error parsing dehacked lump");
    }

    return 1;
}

int DEH_LoadLumpByName(char *name, boolean allow_long, boolean allow_error)
{
    int lumpnum;

    lumpnum = W_CheckNumForName(name);

    if (lumpnum == -1)
    {
        fprintf(stderr, "DEH_LoadLumpByName: '%s' lump not found\n", name);
        return 0;
    }

    return DEH_LoadLump(lumpnum, allow_long, allow_error);
}

// Check the command line for -deh argument, and others.
void DEH_ParseCommandLine(void)
{
    char *filename;
    int p;

    //!
    // @arg <files>
    // @category mod
    //
    // Load the given dehacked patch(es)
    //

    p = M_CheckParm("-deh");

    if (p > 0)
    {
        ++p;

        while (p < myargc && myargv[p][0] != '-')
        {
            filename = W_FindWADByName(myargv[p]);
            DEH_LoadFile(filename);
            ++p;
        }
    }
}

