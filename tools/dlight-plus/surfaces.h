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

#ifndef __SURFACES_H__
#define __SURFACES_H__

typedef enum
{
    ST_UNKNOWN      = 0,
    ST_MIDDLESEG,
    ST_UPPERSEG,
    ST_LOWERSEG,
    ST_CEILING,
    ST_FLOOR
} surfaceType_t;

typedef kexArray<kexVec3> vertexBatch_t;

// convert from fixed point(FRACUNIT) to floating point
#define F(x)  (((float)(x))/65536.0f)

typedef struct
{
    kexPlane                plane;
    int                     lightmapNum;
    int                     lightmapOffs[2];
    int                     lightmapDims[2];
    kexVec3                 lightmapOrigin;
    kexVec3                 lightmapSteps[2];
    kexVec3                 textureCoords[2];
    kexBBox                 bounds;
    int                     numVerts;
    kexVec3                 *verts;
    float                   *lightmapCoords;
    surfaceType_t           type;
    int                     typeIndex;
    void                    *data;
    bool                    bSky;
    struct mapSubSector_s   *subSector;
} surface_t;

extern kexArray<surface_t*> surfaces;

class kexDoomMap;
class kexWadFile;

void Surface_AllocateFromMap(kexDoomMap &doomMap);

#endif
