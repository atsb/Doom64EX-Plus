// Emacs style mode select   -*- C -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 2005 Simon Howard
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

#ifndef W_MERGE_H
#define W_MERGE_H

#define W_NWT_MERGE_SPRITES   0x1
#define W_NWT_MERGE_FLATS     0x2

// Add a new WAD and merge it into the main directory

void W_MergeFile(int8_t* filename);

// NWT-style merging

void W_NWTMergeFile(int8_t* filename, int flags);

// Acts the same as NWT's "-merge" option.

void W_NWTDashMerge(int8_t* filename);

// Debug function that prints the WAD directory.

void W_PrintDirectory(void);

#endif /* #ifndef W_MERGE_H */
