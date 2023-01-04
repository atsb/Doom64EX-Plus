// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// Copyright(C) 2013 Samuel Villarreal
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
// DESCRIPTION: Pluecker operations
//              This stuff makes my brain hurt...
//
//-----------------------------------------------------------------------------

#include "mathlib.h"

//
// kexPluecker::kexPluecker
//

kexPluecker::kexPluecker(void) {
    Clear();
}

//
// kexPluecker::kexPluecker
//

kexPluecker::kexPluecker(const kexVec3 &start, const kexVec3 &end, bool bRay) {
    bRay ? SetRay(start, end) : SetLine(start, end);
}

//
// kexPluecker::Clear
//

void kexPluecker::Clear(void) {
    p[0] = p[1] = p[2] = p[3] = p[4] = p[5] = 0;
}

//
// kexPluecker::SetLine
//

void kexPluecker::SetLine(const kexVec3 &start, const kexVec3 &end) {
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

void kexPluecker::SetRay(const kexVec3 &start, const kexVec3 &dir) {
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

float kexPluecker::InnerProduct(const kexPluecker &pluecker) const {
    return
        p[0] * pluecker.p[4] +
        p[1] * pluecker.p[5] +
        p[2] * pluecker.p[3] +
        p[4] * pluecker.p[0] +
        p[5] * pluecker.p[1] +
        p[3] * pluecker.p[2];
}
