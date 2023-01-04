//
// Copyright (c) 2013-2014 Samuel Villarreal
// svkaiser@gmail.com
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
//    1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
//
//   2. Altered source versions must be plainly marked as such, and must not be
//   misrepresented as being the original software.
//
//    3. This notice may not be removed or altered from any source
//    distribution.
//
//-----------------------------------------------------------------------------
//
// DESCRIPTION: Random operations
//
//-----------------------------------------------------------------------------

#include <math.h>
#include "mathlib.h"

#define RANDOM_MAX  0x7FFF

int kexRand::seed = 0;

//
// kexRand::SetSeed
//

void kexRand::SetSeed(const int randSeed)
{
    seed = randSeed;
}

//
// kexRand::SysRand
//

int kexRand::SysRand(void)
{
    return rand();
}

//
// kexRand::Int
//

int kexRand::Int(void)
{
    seed = 1479838765 - 1471521965 * seed;
    return seed & RANDOM_MAX;
}

//
// kexRand::Max
//

int kexRand::Max(const int max)
{
    if(max == 0)
    {
        return 0;
    }

    return Int() % max;
}

//
// kexRand::Float
//

float kexRand::Float(void)
{
    return (float)Max(RANDOM_MAX+1) / ((float)RANDOM_MAX+1);
}

//
// kexRand::CFloat
//

float kexRand::CFloat(void)
{
    return (float)(Max(20000) - 10000) * 0.0001f;
}

