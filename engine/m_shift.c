// Emacs style mode select   -*- C -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 1999-2000 Paul Brook
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
// DESCRIPTION:
//		shift transformation tables
// 
//-----------------------------------------------------------------------------

#include "doomstat.h"

const char* shiftxform;

const char english_shiftxform[] = {
	0,
	1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
	11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
	21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
	31,
	' ', '!', '"', '#', '$', '%', '&',
	'"', // shift-'
	'(', ')', '*', '+',
	'<', // shift-,
	'_', // shift--
	'>', // shift-.
	'?', // shift-/
	')', // shift-0
	'!', // shift-1
	'@', // shift-2
	'#', // shift-3
	'$', // shift-4
	'%', // shift-5
	'^', // shift-6
	'&', // shift-7
	'*', // shift-8
	'(', // shift-9
	':',
	':', // shift-;
	'<',
	'+', // shift-=
	'>', '?', '@',
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
	'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
	'[', // shift-[
	'!', // shift-backslash - OH MY GOD DOES WATCOM SUCK
	']', // shift-]
	'"', '_',
	'\'', // shift-`
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
	'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
	'{', '|', '}', '~', 127
};

void M_InitShiftXForm(void) {
	shiftxform = english_shiftxform;
}
