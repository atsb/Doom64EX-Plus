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
// DESCRIPTION: Pluecker operations
//              This stuff makes my brain hurt...
//
//-----------------------------------------------------------------------------

#include <math.h>
#include "mathlib.h"

//
// kexPluecker::kexPluecker
//

kexPluecker::kexPluecker(void)
{
    Clear();
}

//
// kexPluecker::kexPluecker
//

kexPluecker::kexPluecker(const kexVec3 &start, const kexVec3 &end, bool bRay)
{
    bRay ? SetRay(start, end) : SetLine(start, end);
}

//
// kexPluecker::Clear
//

void kexPluecker::Clear(void)
{
    p[0] = p[1] = p[2] = p[3] = p[4] = p[5] = 0;
}

//
// kexPluecker::SetLine
//

void kexPluecker::SetLine(const kexVec3 &start, const kexVec3 &end)
{
    p[0] = start.x * end.y - end.x * start.y;
    p[1] = start.x * end.z - end.x * start.z;
    p[3] = start.y * end.z - end.y * start.z;

    p[2] = start.x - end.x;
    p[5] = end.y - start.y;
    p[4] = start.z - end.z;
}

//
// kexPluecker::SetRay
//

void kexPluecker::SetRay(const kexVec3 &start, const kexVec3 &dir)
{
    p[0] = start.x * dir.y - dir.x * start.y;
    p[1] = start.x * dir.z - dir.x * start.z;
    p[3] = start.y * dir.z - dir.y * start.z;

    p[2] = -dir.x;
    p[5] = dir.y;
    p[4] = -dir.z;
}

//
// kexPluecker::InnerProduct
//

float kexPluecker::InnerProduct(const kexPluecker &pluecker) const
{
    return
        p[0] * pluecker.p[4] +
        p[1] * pluecker.p[5] +
        p[2] * pluecker.p[3] +
        p[4] * pluecker.p[0] +
        p[5] * pluecker.p[1] +
        p[3] * pluecker.p[2];
}
