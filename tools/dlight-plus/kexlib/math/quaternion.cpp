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
// DESCRIPTION: Quaternion operations
//
//-----------------------------------------------------------------------------

#include <math.h>
#include "mathlib.h"

//
// kexQuat::kexQuat
//

kexQuat::kexQuat(void)
{
    Clear();
}

//
// kexQuat::kexQuat
//

kexQuat::kexQuat(const float angle, const float x, const float y, const float z)
{
    float s = kexMath::Sin(angle * 0.5f);
    float c = kexMath::Cos(angle * 0.5f);

    this->x = x * s;
    this->y = y * s;
    this->z = z * s;
    this->w = c;
}

//
// kexQuat::kexQuat
//

kexQuat::kexQuat(const float angle, kexVec3 &vector)
{
    float s = kexMath::Sin(angle * 0.5f);
    float c = kexMath::Cos(angle * 0.5f);

    this->x = vector.x * s;
    this->y = vector.y * s;
    this->z = vector.z * s;
    this->w = c;
}

//
// kexQuat::kexQuat
//

kexQuat::kexQuat(const float angle, const kexVec3 &vector)
{
    float s = kexMath::Sin(angle * 0.5f);
    float c = kexMath::Cos(angle * 0.5f);

    this->x = vector.x * s;
    this->y = vector.y * s;
    this->z = vector.z * s;
    this->w = c;
}

//
// kexQuat::Set
//

void kexQuat::Set(const float x, const float y, const float z, const float w)
{
    this->x = x;
    this->y = y;
    this->z = z;
    this->w = w;
}

//
// kexQuat::Clear
//

void kexQuat::Clear(void)
{
    x = y = z = 0.0f;
    w = 1.0f;
}

//
// kexVec3::UnitSq
//

float kexQuat::UnitSq(void) const
{
    return x * x + y * y + z * z + w * w;
}

//
// kexVec3::Unit
//

float kexQuat::Unit(void) const
{
    return kexMath::Sqrt(UnitSq());
}

//
// kexQuat::Normalize
//

kexQuat &kexQuat::Normalize(void)
{
    float d = Unit();
    if(d != 0.0f)
    {
        d = 1.0f / d;
        *this *= d;
    }
    return *this;
}

//
// kexQuat::Inverse
//

kexQuat kexQuat::Inverse(void) const
{
    kexQuat out;
    out.Set(-x, -y, -z, -w);
    return out;
}

//
// kexQuat::operator+
//

kexQuat kexQuat::operator+(const kexQuat &quat)
{
    kexQuat out;
    out.x = x + quat.x;
    out.y = y + quat.y;
    out.z = z + quat.z;
    out.w = w + quat.w;
    return out;
}

//
// kexQuat::operator+=
//

kexQuat &kexQuat::operator+=(const kexQuat &quat)
{
    x += quat.x;
    y += quat.y;
    z += quat.z;
    w += quat.w;
    return *this;
}

//
// kexQuat::operator-
//

kexQuat kexQuat::operator-(const kexQuat &quat)
{
    kexQuat out;
    out.x = x - quat.x;
    out.y = y - quat.y;
    out.z = z - quat.z;
    out.w = w - quat.w;
    return out;
}

//
// kexQuat::operator-=
//

kexQuat &kexQuat::operator-=(const kexQuat &quat)
{
    x -= quat.x;
    y -= quat.y;
    z -= quat.z;
    w -= quat.w;
    return *this;
}

//
// kexQuat::operator*
//

kexQuat kexQuat::operator*(const kexQuat &quat)
{
    kexQuat out;

    out.x = x * quat.w - y * quat.z + quat.x * w + quat.y * z;
    out.y = x * quat.z + y * quat.w - quat.x * z + w * quat.y;
    out.z = quat.x * y + w * quat.z + z * quat.w - x * quat.y;
    out.w = w * quat.w - quat.y * y + z * quat.z + quat.x * x;

    return out;
}

//
// kexQuat::operator*=
//

kexQuat &kexQuat::operator*=(const kexQuat &quat)
{
    float tx = x;
    float ty = y;
    float tz = z;
    float tw = w;

    x = tx * quat.w - ty * quat.z + quat.x * tw + quat.y * z;
    y = tx * quat.z + ty * quat.w - quat.x * tz + tw * quat.y;
    z = quat.x * ty + tw * quat.z + tz * quat.w - tx * quat.y;
    w = tw * quat.w - quat.y * ty + tz * quat.z + quat.x * x;

    return *this;
}

//
// kexQuat::operator*
//

kexQuat kexQuat::operator*(const float val) const
{
    kexQuat out;
    out.x = x * val;
    out.y = y * val;
    out.z = z * val;
    out.w = w * val;
    return out;
}

//
// kexQuat::operator*=
//

kexQuat &kexQuat::operator*=(const float val)
{
    x *= val;
    y *= val;
    z *= val;
    w *= val;

    return *this;
}

//
// kexQuat::operator|
//

kexVec3 kexQuat::operator|(const kexVec3 &vector)
{
    float xx = x * x;
    float yx = y * x;
    float zx = z * x;
    float wx = w * x;
    float yy = y * y;
    float zy = z * y;
    float wy = w * y;
    float zz = z * z;
    float wz = w * z;
    float ww = w * w;

    return kexVec3(
               ((yx + yx) - (wz + wz)) * vector.y +
               ((wy + wy + zx + zx)) * vector.z +
               (((ww + xx) - yy) - zz) * vector.x,
               ((yy + (ww - xx)) - zz) * vector.y +
               ((zy + zy) - (wx + wx)) * vector.z +
               ((wz + wz) + (yx + yx)) * vector.x,
               ((wx + wx) + (zy + zy)) * vector.y +
               (((ww - xx) - yy) + zz) * vector.z +
               ((zx + zx) - (wy + wy)) * vector.x
           );
}

//
// kexQuat::Dot
//

float kexQuat::Dot(const kexQuat &quat) const
{
    return (x * quat.x + y * quat.y + z * quat.z + w * quat.w);
}

//
// kexQuat::Slerp
//

kexQuat kexQuat::Slerp(const kexQuat &quat, float movement) const
{
    kexQuat rdest = quat;
    float d1 = Dot(quat);
    float d2 = Dot(quat.Inverse());

    if(d1 < d2)
    {
        rdest = quat.Inverse();
        d1 = d2;
    }

    if(d1 <= 0.7071067811865001f)
    {
        float halfcos = kexMath::ACos(d1);
        float halfsin = kexMath::Sin(halfcos);

        if(halfsin == 0)
        {
            kexQuat out;
            out.Set(x, y, z, w);
            return out;
        }
        else
        {
            float d;
            float ms1;
            float ms2;

            d = 1.0f / halfsin;
            ms1 = kexMath::Sin((1.0f - movement) * halfcos) * d;
            ms2 = kexMath::Sin(halfcos * movement) * d;

            if(ms2 < 0)
            {
                rdest = quat.Inverse();
            }

            return *this * ms1 + rdest * ms2;
        }
    }
    else
    {
        kexQuat out = (rdest - *this) * movement + *this;
        out.Normalize();
        return out;
    }
}

//
// kexQuat::RotateFrom
//

kexQuat kexQuat::RotateFrom(const kexVec3 &location, const kexVec3 &target, float maxAngle)
{
    kexVec3 axis;
    kexVec3 dir;
    kexVec3 cp;
    kexQuat prot;
    float an;

    dir = (*this | kexVec3::vecForward);
    axis = (target - location).Normalize();
    cp = dir.Cross(axis).Normalize();

    an = kexMath::ACos(axis.Dot(dir));

    if(maxAngle != 0 && an >= maxAngle)
    {
        an = maxAngle;
    }

    return (*this * kexQuat(an, cp));
}

//
// kexQuat::operator=
//

kexQuat &kexQuat::operator=(const kexQuat &quat)
{
    x = quat.x;
    y = quat.y;
    z = quat.z;
    w = quat.w;
    return *this;
}

//
// kexQuat::operator=
//

kexQuat &kexQuat::operator=(const kexVec4 &vec)
{
    x = vec.x;
    y = vec.y;
    z = vec.z;
    w = vec.w;
    return *this;
}

//
// kexQuat::operator=
//

kexQuat &kexQuat::operator=(const float *vecs)
{
    x = vecs[0];
    y = vecs[1];
    z = vecs[2];
    w = vecs[3];
    return *this;
}

//
// kexQuat::ToVec3
//

kexVec3 const &kexQuat::ToVec3(void) const
{
    return *reinterpret_cast<const kexVec3*>(this);
}

//
// kexQuat::ToVec3
//

kexVec3 &kexQuat::ToVec3(void)
{
    return *reinterpret_cast<kexVec3*>(this);
}
