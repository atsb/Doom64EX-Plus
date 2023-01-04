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
// DESCRIPTION: Math functions
//
//-----------------------------------------------------------------------------

#include "mathlib.h"

//
// kexMath::Abs
//

int kexMath::Abs(int x) {
    int y = x >> 31;
    return ((x ^ y) - y);
}

//
// kexMath::Fabs
//

float kexMath::Fabs(float x) {
    int tmp = *reinterpret_cast<int*>(&x);
    tmp &= 0x7FFFFFFF;
    return *reinterpret_cast<float*>(&tmp);
}

//
// kexMath::RoundPowerOfTwo
//

int kexMath::RoundPowerOfTwo(int x) {
    int mask = 1;
    
    while(mask < 0x40000000) {
        if(x == mask || (x & (mask-1)) == x) {
            return mask;
        }
        
        mask <<= 1;
    }
    
    return x;
}

//
// kexMath::InvSqrt
//

float kexMath::InvSqrt(float x) {
    unsigned int i;
    float r;
    float y;
    
    y = x * 0.5f;
    i = *reinterpret_cast<unsigned int*>(&x);
    i = 0x5f3759df - (i >> 1);
    r = *reinterpret_cast<float*>(&i);
    r = r * (1.5f - r * r * y);
    
    return r;
}

//
// kexMath::SinZeroHalfPI
// Apapted from http://mrelusive.com/publications/papers/SIMD-Slerping-Clock-Cycles.pdf
//

float kexMath::SinZeroHalfPI(float a) {
    float s, t;
    
    s = a * a;
    t = -2.39e-08f;
    t *= s;
    t += 2.7526e-06f;
    t *= s;
    t += -1.98409e-04f;
    t *= s;
    t += 8.3333315e-03f;
    t *= s;
    t += -1.666666664e-01f;
    t *= s;
    t += 1.0f;
    t *= a;
    
    return t;
}

//
// kexMath::ATanPositive
// Apapted from http://mrelusive.com/publications/papers/SIMD-Slerping-Clock-Cycles.pdf
//

float kexMath::ATanPositive(float y, float x) {
    float a, d, s, t;
    
    if(y > x) {
        a = -x / y;
        d = M_PI / 2;
    }
    else {
        a = y / x;
        d = 0.0f;
    }
    s = a * a;
    t = 0.0028662257f;
    t *= s;
    t += -0.0161657367f;
    t *= s;
    t += 0.0429096138f;
    t *= s;
    t += -0.0752896400f;
    t *= s;
    t += 0.1065626393f;
    t *= s;
    t += -0.1420889944f;
    t *= s;
    t += 0.1999355085f;
    t *= s;
    t += -0.3333314528f;
    t *= s;
    t += 1.0f;
    t *= a;
    t += d;
    
    return t;
}

//
// kexMath::FCmp
//

bool kexMath::FCmp(float f1, float f2) {
    float precision = 0.00001f;
    if(((f1 - precision) < f2) && 
        ((f1 + precision) > f2)) {
        return true;
    }
    else {
        return false;
    }
}

//
// kexMath::Clamp
//

void kexMath::Clamp(float &f, const float min, const float max) {
    if(f < min) f = min;
    if(f > max) f = max;
}

//
// kexMath::Clamp
//

void kexMath::Clamp(int &i, const int min, const int max) {
    if(i < min) i = min;
    if(i > max) i = max;
}

//
// kexMath::Clamp
//

void kexMath::Clamp(byte &b, const byte min, const byte max) {
    if(b < min) b = min;
    if(b > max) b = max;
}

//
// kexMath::CubicCurve
//

void kexMath::CubicCurve(const kexVec3 &start, const kexVec3 &end, const float time,
                         const kexVec3 &point, kexVec3 *vec) {
    int i;
    float xyz[3];

    for(i = 0; i < 3; i++) {
        xyz[i] = kexMath::Pow(1-time, 2) * start[i] +
            (2 * (1-time)) * time * point[i] + kexMath::Pow(time, 2) * end[i];
    }

    vec->x = xyz[0];
    vec->y = xyz[1];
    vec->z = xyz[2];
}

//
// kexMath::QuadraticCurve
//

void kexMath::QuadraticCurve(const kexVec3 &start, const kexVec3 &end, const float time,
                             const kexVec3 &pt1, const kexVec3 &pt2, kexVec3 *vec) {
    int i;
    float xyz[3];

    for(i = 0; i < 3; i++) {
        xyz[i] = kexMath::Pow(1-time, 3) * start[i] + (3 * kexMath::Pow(1-time, 2)) *
            time * pt1[i] + (3 * (1-time)) * kexMath::Pow(time, 2) * pt2[i] +
            kexMath::Pow(time, 3) * end[i];
    }

    vec->x = xyz[0];
    vec->y = xyz[1];
    vec->z = xyz[2];
}
