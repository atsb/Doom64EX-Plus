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
// DESCRIPTION: Angle operations
//
//-----------------------------------------------------------------------------

#include "mathlib.h"

#define FULLCIRCLE  (M_PI * 2)

//
// kexAngle::kexAngle
//

kexAngle::kexAngle(void) {
    this->yaw   = 0;
    this->pitch = 0;
    this->roll  = 0;
}

//
// kexAngle::kexAngle
//

kexAngle::kexAngle(const float yaw, const float pitch, const float roll) {
    this->yaw   = yaw;
    this->pitch = pitch;
    this->roll  = roll;
}

//
// kexAngle::kexAngle
//

kexAngle::kexAngle(const kexVec3 &vector) {
    this->yaw   = vector.x;
    this->pitch = vector.y;
    this->roll  = vector.z;

    Clamp180();
}

//
// kexAngle::kexAngle
//

kexAngle::kexAngle(const kexAngle &an) {
    this->yaw   = an.yaw;
    this->pitch = an.pitch;
    this->roll  = an.roll;
}

//
// kexAngle::Round
//

float kexAngle::Round(float angle) {
    return DEG2RAD((360.0f / 65536.0f) *
        ((int)(RAD2DEG(angle) * (65536.0f / 360.0f)) & 65535));
}

//
// kexAngle::Clamp
//

void kexAngle::Clamp(float *angle) {
    float an = *angle;

    if(an < -M_PI) for(; an < -M_PI; an = an + FULLCIRCLE);
    if(an >  M_PI) for(; an >  M_PI; an = an - FULLCIRCLE);

    *angle = an;
}

//
// kexAngle::ClampInvert
//

float kexAngle::ClampInvert(float angle) {
    float an = angle;
    
    if(an < -M_PI) for(; an < -M_PI; an = an + FULLCIRCLE);
    if(an >  M_PI) for(; an >  M_PI; an = an - FULLCIRCLE);

    return -an;
}

//
// kexAngle::ClampInvertSums
//

float kexAngle::ClampInvertSums(float angle1, float angle2) {
    return ClampInvert(ClampInvert(angle2) + angle1);
}

//
// kexAngle::DiffAngles
//

float kexAngle::DiffAngles(float angle1, float angle2) {
    float an1;
    float an2;

    Clamp(&angle1);
    Clamp(&angle2);

    an2 = 0.0f;

    if(angle1 <= angle2) {
        an1 = angle2 + FULLCIRCLE;
        if(angle1 - angle2 > an1 - angle1) {
            an2 = angle1 - an1;
        }
        else {
            an2 = angle1 - angle2;
        }
    }
    else {
        an1 = angle2 - FULLCIRCLE;
        if(angle2 - angle1 <= angle1 - an1) {
            an2 = angle1 - angle2;
        }
        else {
            an2 = angle1 - an1;
        }
    }

    return an2;
}

//
// kexAngle::Clamp180
//

kexAngle &kexAngle::Clamp180(void) {
    Clamp(&yaw);
    Clamp(&pitch);
    Clamp(&roll);
    return *this;
}

//
// kexAngle::Clamp180Invert
//

kexAngle &kexAngle::Clamp180Invert(void) {
    yaw = ClampInvert(yaw);
    pitch = ClampInvert(pitch);
    roll = ClampInvert(roll);

    return *this;
}

//
// kexAngle::Clamp180InvertSum
//

kexAngle &kexAngle::Clamp180InvertSum(const kexAngle &angle) {
    yaw = ClampInvertSums(yaw, angle.yaw);
    pitch = ClampInvertSums(pitch, angle.pitch);
    roll = ClampInvertSums(roll, angle.roll);

    return *this;
}

//
// kexAngle::Round
//

kexAngle &kexAngle::Round(void) {
    yaw     = Round(yaw);
    pitch   = Round(pitch);
    roll    = Round(roll);
    return Clamp180();
}

//
// kexAngle::Diff
//

kexAngle kexAngle::Diff(kexAngle &angle) {
    kexAngle out;

    out.yaw = DiffAngles(yaw, angle.yaw);
    out.pitch = DiffAngles(pitch, angle.pitch);
    out.roll = DiffAngles(roll, angle.roll);

    return out;
}

//
// kexAngle::ToAxis
//

void kexAngle::ToAxis(kexVec3 *forward, kexVec3 *up, kexVec3 *right) const {
    float sy = kexMath::Sin(yaw);
    float cy = kexMath::Cos(yaw);
    float sp = kexMath::Sin(pitch);
    float cp = kexMath::Cos(pitch);
    float sr = kexMath::Sin(roll);
    float cr = kexMath::Cos(roll);

    if(forward) {
        forward->x  = sy * cp;
        forward->y  = -sp;
        forward->z  = cy * cp;
    }
    if(right) {
        right->x    = sr * sp * sy + cr * cy;
        right->y    = sr * cp;
        right->z    = sr * sp * cy + cr * -sy;
    }
    if(up) {
        up->x       = cr * sp * sy + -sr * cy;
        up->y       = cr * cp;
        up->z       = cr * sp * cy + -sr * -sy;
    }
}

//
// kexAngle::ToForwardAxis
//

kexVec3 kexAngle::ToForwardAxis(void) const {
    kexVec3 vec;

    ToAxis(&vec, NULL, NULL);
    return vec;
}

//
// kexAngle::ToUpAxis
//

kexVec3 kexAngle::ToUpAxis(void) const {
    kexVec3 vec;

    ToAxis(NULL, &vec, NULL);
    return vec;
}

//
// kexAngle::ToRightAxis
//

kexVec3 kexAngle::ToRightAxis(void) const {
    kexVec3 vec;

    ToAxis(NULL, NULL, &vec);
    return vec;
}

//
// kexAngle::ToVec3
//

const kexVec3 &kexAngle::ToVec3(void) const {
    return *reinterpret_cast<const kexVec3*>(&yaw);
}

//
// kexAngle::ToVec3
//

kexVec3 &kexAngle::ToVec3(void) {
    return *reinterpret_cast<kexVec3*>(&yaw);
}

//
// kexAngle::ToQuat
//

kexQuat kexAngle::ToQuat(void) {
    return 
        (kexQuat(pitch, kexVec3::vecRight) *
        (kexQuat(yaw, kexVec3::vecUp) *
         kexQuat(roll, kexVec3::vecForward)));
}

//
// kexAngle::operator+
//

kexAngle kexAngle::operator+(const kexAngle &angle) {
    return kexAngle(yaw + angle.yaw, pitch + angle.pitch, roll + angle.roll);
}

//
// kexAngle::operator-
//

kexAngle kexAngle::operator-(const kexAngle &angle) {
    return kexAngle(yaw - angle.yaw, pitch - angle.pitch, roll - angle.roll);
}

//
// kexAngle::operator-
//

kexAngle kexAngle::operator-(void) {
    return kexAngle(-yaw, -pitch, -roll);
}

//
// kexAngle::operator+=
//

kexAngle &kexAngle::operator+=(const kexAngle &angle) {
    yaw     += angle.yaw;
    pitch   += angle.pitch;
    roll    += angle.roll;
    return *this;
}

//
// kexAngle::operator-=
//

kexAngle &kexAngle::operator-=(const kexAngle &angle) {
    yaw     -= angle.yaw;
    pitch   -= angle.pitch;
    roll    -= angle.roll;
    return *this;
}

//
// kexAngle::operator=
//

kexAngle &kexAngle::operator=(const kexAngle &angle) {
    yaw     = angle.yaw;
    pitch   = angle.pitch;
    roll    = angle.roll;
    return *this;
}

//
// kexAngle::operator=
//

kexAngle &kexAngle::operator=(const kexVec3 &vector) {
    yaw     = vector.x;
    pitch   = vector.y;
    roll    = vector.z;
    return *this;
}

//
// kexAngle::operator=
//

kexAngle &kexAngle::operator=(const float *vecs) {
    yaw     = vecs[0];
    pitch   = vecs[1];
    roll    = vecs[2];
    return *this;
}

//
// kexAngle::operator[]
//

float kexAngle::operator[](int index) const {
    assert(index >= 0 && index < 3);
    return (&yaw)[index];
}

//
// kexAngle::operator[]
//

float &kexAngle::operator[](int index) {
    assert(index >= 0 && index < 3);
    return (&yaw)[index];
}
