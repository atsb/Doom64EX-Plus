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

#ifndef __LIGHT_SURFACE_H__
#define __LIGHT_SURFACE_H__

#include "surfaces.h"

typedef struct
{
    int             tag;
    float           outerCone;
    float           innerCone;
    float           falloff;
    float           distance;
    float           intensity;
    bool            bIgnoreFloor;
    bool            bIgnoreCeiling;
    bool            bNoCenterPoint;
    kexVec3         rgb;
} surfaceLightDef_t;

class kexDoomMap;
class kexTrace;

class kexLightSurface
{
public:
    kexLightSurface(void);
    ~kexLightSurface(void);

    void                    Init(const surfaceLightDef_t &lightSurfaceDef, surface_t *surface,
                                 const bool bWall, const bool bNoCenterPoint);
    void                    Subdivide(const float divide);
    void                    CreateCenterOrigin(void);
    bool                    TraceSurface(kexDoomMap *doomMap, kexTrace &trace, const surface_t *surface,
                                         const kexVec3 &origin, float *dist);

    const float             OuterCone(void) const { return outerCone; }
    const float             InnerCone(void) const { return innerCone; }
    const float             FallOff(void) const { return falloff; }
    const float             Distance(void) const { return distance; }
    const float             Intensity(void) const { return intensity; }
    const kexVec3           GetRGB(void) const { return rgb; }
    const bool              IsAWall(void) const { return bWall; }
    const bool              NoCenterPoint(void) const { return bNoCenterPoint; }
    const surface_t         *Surface(void) const { return surface; }
    const vertexBatch_t     Origins(void) const { return origins; }

private:
    bool                    SubdivideRecursion(vertexBatch_t &surfPoints, float divide,
            kexArray<vertexBatch_t*> &points);
    void                    Clip(vertexBatch_t &points, const kexVec3 &normal, float dist,
                                 vertexBatch_t *frontPoints, vertexBatch_t *backPoints);

    float                   outerCone;
    float                   innerCone;
    float                   falloff;
    float                   distance;
    float                   intensity;
    kexVec3                 rgb;
    bool                    bWall;
    bool                    bNoCenterPoint;
    vertexBatch_t           origins;
    surface_t               *surface;
};

#endif
