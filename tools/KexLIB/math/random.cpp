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
//
// DESCRIPTION: Random operations
//
//-----------------------------------------------------------------------------

#include <stdlib.h>
#include "mathlib.h"

#define RANDOM_MAX  0x7FFF
#define RANGE_MAX   10000
#define RANGE_DET   (1.0f / (float)RANGE_MAX)

int kexRand::seed = 0;

//
// kexRand::SetSeed
//

void kexRand::SetSeed(const int randSeed) {
    seed = randSeed;
}

//
// kexRand::SysRand
//

int kexRand::SysRand(void) {
    return rand();
}

//
// kexRand::Int
//

int kexRand::Int(void) {
    seed = 1479838765 - 1471521965 * seed;
    return seed & RANDOM_MAX;
}

//
// kexRand::Max
//

int kexRand::Max(const int max) {
    if(max == 0) {
        return 0;
    }
        
    return Int() % max;
}

//
// kexRand::Float
//

float kexRand::Float(void) {
    return (float)Max(RANDOM_MAX+1) / ((float)RANDOM_MAX+1);
}

//
// kexRand::CFloat
//

float kexRand::CFloat(void) {
    return (float)(Max((RANGE_MAX+RANGE_MAX)) - RANGE_MAX) * RANGE_DET;
}

//
// kexRand::Range
//

float kexRand::Range(const float r1, const float r2) {
    float f = (float)Max(RANGE_MAX) * RANGE_DET;
    return (1.0f - f) * r2 + f * r1;
}

