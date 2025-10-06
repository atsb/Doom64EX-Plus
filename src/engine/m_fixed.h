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

#ifndef __M_FIXED__
#define __M_FIXED__

//
// Fixed point, 32bit as 16.16.
//
#define FRACBITS        16
#define FRACUNIT        (1<<FRACBITS)

#define INT2F(x)        ((x)<<FRACBITS)
#define F2INT(x)        ((x)>>FRACBITS)
#define FLOATTOFIXED(x) ((fixed_t)((x)*FRACUNIT))
#define F2D3D(x)         (((float)(x))/FRACUNIT)

typedef int fixed_t;

fixed_t FixedMul(fixed_t a, fixed_t b);
fixed_t FixedDiv(fixed_t a, fixed_t b);
fixed_t FixedDiv2(fixed_t a, fixed_t b);
fixed_t FixedDot(fixed_t a1, fixed_t b1, fixed_t c1, fixed_t a2, fixed_t b2, fixed_t c2);

#endif
