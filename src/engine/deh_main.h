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
// Dehacked entrypoint and common code
//

#ifndef DEH_MAIN_H
#define DEH_MAIN_H

#include "i_opndir.h"
#include "doomtype.h"
#include "sha1.h"

void DEH_ParseCommandLine(void);
int DEH_LoadFile(char *filename);
int DEH_LoadLump(int lumpnum, bool allow_long, bool allow_error);
int DEH_LoadLumpByName(char *name, bool allow_long, bool allow_error);

bool DEH_ParseAssignment(char *line, char **variable_name, char **value);

void DEH_Checksum(sha1_digest_t digest);

#endif /* #ifndef DEH_MAIN_H */

