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

#ifndef __LIGHTMAP_H__
#define __LIGHTMAP_H__

#include "surfaces.h"

#define LIGHTMAP_MAX_SIZE  1024

class kexTrace;

class kexLightmapBuilder
{
public:
    kexLightmapBuilder(void);
    ~kexLightmapBuilder(void);

    void                    BuildSurfaceParams(surface_t *surface);
    void                    TraceSurface(surface_t *surface);
    void                    CreateLightGrid(void);
    void                    CreateLightmaps(kexDoomMap &doomMap);
    void                    LightSurface(const int surfid);
    void                    LightGrid(const int gridid);
    void                    WriteTexturesToTGA(void);
    void                    AddLightGridLump(kexWadFile &wadFile);
    void                    AddLightmapLumps(kexWadFile &wadFile);

    int                     samples;
    float                   ambience;
    int                     textureWidth;
    int                     textureHeight;

    static const kexVec3    gridSize;

private:
    void                    NewTexture(void);
    bool                    MakeRoomForBlock(const int width, const int height, int *x, int *y, int *num);
    kexBBox                 GetBoundsFromSurface(const surface_t *surface);
    kexVec3                 LightTexelSample(kexTrace &trace, const kexVec3 &origin, surface_t *surface);
    kexVec3                 LightCellSample(const int gridid, kexTrace &trace,
                                            const kexVec3 &origin, const mapSubSector_t *sub);
    bool                    EmitFromCeiling(kexTrace &trace, const surface_t *surface, const kexVec3 &origin,
                                            const kexVec3 &normal, float *dist);
    void                    ExportTexelsToObjFile(FILE *f, const kexVec3 &org, int indices);
    void                    WriteBlock(FILE *f, const int i, const kexVec3 &org, int indices, kexBBox &box);

    typedef struct
    {
        byte                marked;
        byte                sunShadow;
        kexVec3             color;
    } gridMap_t;

    kexDoomMap              *map;
    kexArray<byte*>         textures;
    int                     **allocBlocks;
    int                     numTextures;
    int                     extraSamples;
    int                     tracedTexels;
    int                     numLightGrids;
    gridMap_t               *gridMap;
    mapSubSector_t          **gridSectors;
    kexBBox                 worldGrid;
    kexBBox                 gridBound;
    kexVec3                 gridBlock;
};

#endif
