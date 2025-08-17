// Emacs style mode select   -*- C -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 1993-1997 Id Software, Inc.
// Copyright(C) 2007-2012 Samuel Villarreal
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
//-----------------------------------------------------------------------------
//
// DESCRIPTION:
//    Fixed point implementation.
//
//-----------------------------------------------------------------------------

#include <stdint.h>
#include <limits.h>

#include <SDL3/SDL_platform_defines.h>

#include "m_fixed.h"
#include "doomdef.h"

/* ATSB 

This is some preliminary ARM
assembler optimisation for some math routines
Taken from Doom64EX (DS Version).

Meaning:

SMULL - multiplies integers then adds a 32bit result to the 64bit signed int
SMLAL - Same as above but 64bit to 64bit
Move registers (this is the math stuff)
ORR - perform some bitwise operations
BX - branches the instructions and exchanges if need be

*/

fixed_t
FixedMul
(fixed_t    a,
	fixed_t    b) {

#if defined __arm__ && !defined SDL_PLATFORM_MACOS && !defined __GNUC__
	asm(
		"SMULL 	 R2, R3, R0, R1\n\t"
		"MOV	 R1, R2, LSR #16\n\t"
		"MOV	 R2, R3, LSL #16\n\t"
		"ORR	 R0, R1, R2\n\t"
		"BX		 LR"
	);
#elif defined __aarch64__ && !defined SDL_PLATFORM_MACOS && !defined __GNUC__
	asm(
		"SMLAL 	 R2, R3, R0, R1\n\t"
		"MOV	 R1, R2, LSR #16\n\t"
		"MOV	 R2, R3, LSL #16\n\t"
		"ORR	 R0, R1, R2\n\t"
		"BX		 LR"
	);
#else
	return (fixed_t)(((int64_t)a * (int64_t)b) >> FRACBITS);
#endif
}

//
// FixedDiv, C version.
//

fixed_t
FixedDiv
(fixed_t    a,
	fixed_t    b) {
	if ((D_abs(a) >> 14) >= D_abs(b)) {
		return (a ^ b) < 0 ? INT_MIN : INT_MAX;
	}
	return FixedDiv2(a, b);
}

fixed_t
FixedDiv2
(fixed_t    a,
	fixed_t    b) {
	return (fixed_t)((((int64_t)a) << FRACBITS) / b);
}

//
// FixedDot
//

fixed_t FixedDot(fixed_t a1, fixed_t b1,
	fixed_t c1, fixed_t a2,
	fixed_t b2, fixed_t c2) {
	return
		FixedMul(a1, a2) +
		FixedMul(b1, b2) +
		FixedMul(c1, c2);
}
