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
// DESCRIPTION: Lightmap and lightgrid building module
//
//-----------------------------------------------------------------------------

#include "common.h"
#include "surfaces.h"
#include "trace.h"
#include "mapData.h"
#include "lightmap.h"
#include "worker.h"
#include "kexlib/binFile.h"

//#define EXPORT_TEXELS_OBJ

kexWorker lightmapWorker;

const kexVec3 kexLightmapBuilder::gridSize(64, 64, 128);

//
// LightmapWorkerFunc
//

static void LightmapWorkerFunc(void *data, int id)
{
    kexLightmapBuilder *builder = static_cast<kexLightmapBuilder*>(data);
    builder->LightSurface(id);
}

//
// LightGridWorkerFunc
//

static void LightGridWorkerFunc(void *data, int id)
{
    kexLightmapBuilder *builder = static_cast<kexLightmapBuilder*>(data);
    builder->LightGrid(id);
}

//
// kexLightmapBuilder::kexLightmapBuilder
//

kexLightmapBuilder::kexLightmapBuilder(void)
{
    this->textureWidth  = 128;
    this->textureHeight = 128;
    this->allocBlocks   = NULL;
    this->numTextures   = 0;
    this->samples       = 16;
    this->extraSamples  = 2;
    this->ambience      = 0.0f;
    this->tracedTexels  = 0;
    this->gridMap       = NULL;
}

//
// kexLightmapBuilder::~kexLightmapBuilder
//

kexLightmapBuilder::~kexLightmapBuilder(void)
{
}

//
// kexLightmapBuilder::NewTexture
//
// Allocates a new texture pointer
//

void kexLightmapBuilder::NewTexture(void)
{
    numTextures++;

    allocBlocks = (int**)Mem_Realloc(allocBlocks, sizeof(int*) * numTextures, hb_static);
    allocBlocks[numTextures-1] = (int*)Mem_Calloc(sizeof(int) * textureWidth, hb_static);

    memset(allocBlocks[numTextures-1], 0, sizeof(int) * textureWidth);

    byte *texture = (byte*)Mem_Calloc((textureWidth * textureHeight) * 3, hb_static);
    textures.Push(texture);
}

//
// kexLightMapBuilder::MakeRoomForBlock
//
// Determines where to map a new block on to
// the lightmap texture
//

bool kexLightmapBuilder::MakeRoomForBlock(const int width, const int height,
        int *x, int *y, int *num)
{
    int i;
    int j;
    int k;
    int bestRow1;
    int bestRow2;

    *num = -1;

    if(allocBlocks == NULL)
    {
        return false;
    }

    for(k = 0; k < numTextures; ++k)
    {
        bestRow1 = textureHeight;

        for(i = 0; i <= textureWidth - width; i++)
        {
            bestRow2 = 0;

            for(j = 0; j < width; j++)
            {
                if(allocBlocks[k][i + j] >= bestRow1)
                {
                    break;
                }

                if(allocBlocks[k][i + j] > bestRow2)
                {
                    bestRow2 = allocBlocks[k][i + j];
                }
            }

            // found a free block
            if(j == width)
            {
                *x = i;
                *y = bestRow1 = bestRow2;
            }
        }

        if(bestRow1 + height > textureHeight)
        {
            // no room
            continue;
        }

        for(i = 0; i < width; i++)
        {
            // store row offset
            allocBlocks[k][*x + i] = bestRow1 + height;
        }

        *num = k;
        return true;
    }

    return false;
}

//
// kexLightmapBuilder::GetBoundsFromSurface
//

kexBBox kexLightmapBuilder::GetBoundsFromSurface(const surface_t *surface)
{
    kexVec3 low(M_INFINITY, M_INFINITY, M_INFINITY);
    kexVec3 hi(-M_INFINITY, -M_INFINITY, -M_INFINITY);

    kexBBox bounds;
    bounds.Clear();

    for(int i = 0; i < surface->numVerts; i++)
    {
        for(int j = 0; j < 3; j++)
        {
            if(surface->verts[i][j] < low[j])
            {
                low[j] = surface->verts[i][j];
            }
            if(surface->verts[i][j] > hi[j])
            {
                hi[j] = surface->verts[i][j];
            }
        }
    }

    bounds.min = low;
    bounds.max = hi;

    return bounds;
}

//
// kexLightmapBuilder::EmitFromCeiling
//
// Traces to the ceiling surface. Will emit
// light if the surface that was traced is a sky
//

bool kexLightmapBuilder::EmitFromCeiling(kexTrace &trace, const surface_t *surface, const kexVec3 &origin,
        const kexVec3 &normal, float *dist)
{
    *dist = normal.Dot(map->GetSunDirection());

    if(*dist <= 0)
    {
        // plane is not even facing the sunlight
        return false;
    }

    trace.Trace(origin, origin + (map->GetSunDirection() * 32768));

    if(trace.fraction == 1 || trace.hitSurface == NULL)
    {
        // nothing was hit
        return false;
    }

    if(trace.hitSurface->bSky == false)
    {
        // not a ceiling/sky surface
        return false;
    }

    return true;
}

//
// kexLightmapBuilder::LightTexelSample
//
// Traces a line from the texel's origin to the sunlight direction
// and against all nearby thing lights
//

kexVec3 kexLightmapBuilder::LightTexelSample(kexTrace &trace, const kexVec3 &origin, surface_t *surface)
{
    kexVec3 lightOrigin;
    kexVec3 dir;
    kexVec3 color;
    kexPlane plane;
    float dist;
    float radius;
    float intensity;
    float colorAdd;

    plane = surface->plane;
    color.Clear();

    // check all thing lights
    for(unsigned int i = 0; i < map->thingLights.Length(); i++)
    {
        thingLight_t *tl = map->thingLights[i];

        // try to early out if PVS data exists
        if(!map->CheckPVS(surface->subSector, tl->ssect))
        {
            continue;
        }

        lightOrigin.Set(tl->origin.x,
                        tl->origin.y,
                        !tl->bCeiling ?
                        tl->sector->floorheight + tl->height :
                        tl->sector->ceilingheight - tl->height);

        if(plane.Distance(lightOrigin) - plane.d < 0)
        {
            // completely behind the plane
            continue;
        }

        radius = tl->radius;
        intensity = tl->intensity;

        if(origin.DistanceSq(lightOrigin) > (radius*radius))
        {
            // not within range
            continue;
        }

        trace.Trace(lightOrigin, origin);

        if(trace.fraction != 1)
        {
            // this light is occluded by something
            continue;
        }

        dir = (lightOrigin - origin);
        dist = dir.Unit();

        dir.Normalize();

        float r = MAX(radius - dist, 0);

        colorAdd = ((r * plane.Normal().Dot(dir)) / radius) * intensity;
        kexMath::Clamp(colorAdd, 0, 1);

        if(tl->falloff != 1)
        {
            colorAdd = kexMath::Pow(colorAdd, tl->falloff);
        }

        // accumulate results
        color = color.Lerp(tl->rgb, colorAdd);
        kexMath::Clamp(color, 0, 1);

        tracedTexels++;
    }

    if(surface->type != ST_CEILING && map->bSSectsVisibleToSky[surface->subSector - map->mapSSects])
    {
        // see if it's exposed to sunlight
        if(EmitFromCeiling(trace, surface, origin, plane.Normal(), &dist))
        {
            dist = (dist * 4);
            kexMath::Clamp(dist, 0, 1);

            color = color.Lerp(map->GetSunColor(), dist);
            kexMath::Clamp(color, 0, 1);

            tracedTexels++;
        }
    }

    // trace against surface lights
    for(unsigned int i = 0; i < map->lightSurfaces.Length(); ++i)
    {
        kexLightSurface *surfaceLight = map->lightSurfaces[i];

        // try to early out if PVS data exists
        if(!map->CheckPVS(surface->subSector, surfaceLight->Surface()->subSector))
        {
            continue;
        }

        if(surfaceLight->TraceSurface(map, trace, surface, origin, &dist))
        {
            dist = (dist * surfaceLight->Intensity());
            kexMath::Clamp(dist, 0, 1);

            color = color.Lerp(surfaceLight->GetRGB(), kexMath::Pow(dist, surfaceLight->FallOff()));
            kexMath::Clamp(color, 0, 1);

            tracedTexels++;
        }
    }

    return color;
}

//
// kexLightmapBuilder::BuildSurfaceParams
//
// Determines a lightmap block in which to map to
// the lightmap texture. Width and height of the block
// is calcuated and steps are computed to determine where
// each texel will be positioned on the surface
//

void kexLightmapBuilder::BuildSurfaceParams(surface_t *surface)
{
    kexPlane *plane;
    kexBBox bounds;
    kexVec3 roundedSize;
    int i;
    kexPlane::planeAxis_t axis;
    kexVec3 tCoords[2];
    kexVec3 tOrigin;
    int width;
    int height;
    float d;

    plane = &surface->plane;
    bounds = GetBoundsFromSurface(surface);

    // round off dimentions
    for(i = 0; i < 3; i++)
    {
        bounds.min[i] = samples * kexMath::Floor(bounds.min[i] / samples);
        bounds.max[i] = samples * kexMath::Ceil(bounds.max[i] / samples);

        roundedSize[i] = (bounds.max[i] - bounds.min[i]) / samples + 1;
    }

    tCoords[0].Clear();
    tCoords[1].Clear();

    axis = plane->BestAxis();

    switch(axis)
    {
    case kexPlane::AXIS_YZ:
        width = (int)roundedSize.y;
        height = (int)roundedSize.z;
        tCoords[0].y = 1.0f / samples;
        tCoords[1].z = 1.0f / samples;
        break;

    case kexPlane::AXIS_XZ:
        width = (int)roundedSize.x;
        height = (int)roundedSize.z;
        tCoords[0].x = 1.0f / samples;
        tCoords[1].z = 1.0f / samples;
        break;

    case kexPlane::AXIS_XY:
        width = (int)roundedSize.x;
        height = (int)roundedSize.y;
        tCoords[0].x = 1.0f / samples;
        tCoords[1].y = 1.0f / samples;
        break;
    }

    // clamp width
    if(width > textureWidth)
    {
        tCoords[0] *= ((float)textureWidth / (float)width);
        width = textureWidth;
    }

    // clamp height
    if(height > textureHeight)
    {
        tCoords[1] *= ((float)textureHeight / (float)height);
        height = textureHeight;
    }

    surface->lightmapCoords = (float*)Mem_Calloc(sizeof(float) *
                              surface->numVerts * 2, hb_static);

    surface->textureCoords[0] = tCoords[0];
    surface->textureCoords[1] = tCoords[1];

    tOrigin = bounds.min;

    // project tOrigin and tCoords so they lie on the plane
    d = (plane->Distance(bounds.min) - plane->d) / plane->Normal()[axis];
    tOrigin[axis] -= d;

    for(i = 0; i < 2; i++)
    {
        tCoords[i].Normalize();
        d = plane->Distance(tCoords[i]) / plane->Normal()[axis];
        tCoords[i][axis] -= d;
    }

    surface->bounds = bounds;
    surface->lightmapDims[0] = width;
    surface->lightmapDims[1] = height;
    surface->lightmapOrigin = tOrigin;
    surface->lightmapSteps[0] = tCoords[0] * (float)samples;
    surface->lightmapSteps[1] = tCoords[1] * (float)samples;
}

//
// kexLightmapBuilder::TraceSurface
//
// Steps through each texel and traces a line to the world.
// For each non-occluded trace, color is accumulated and saved off
// into the lightmap texture based on what block is mapped to
//

void kexLightmapBuilder::TraceSurface(surface_t *surface)
{
    kexVec3 colorSamples[256][256];
    int sampleWidth;
    int sampleHeight;
    kexVec3 normal;
    kexVec3 pos;
    kexVec3 tDelta;
    int i;
    int j;
    kexTrace trace;
    byte *currentTexture;
    byte rgb[3];
    bool bShouldLookupTexture = false;

    trace.Init(*map);
    memset(colorSamples, 0, sizeof(colorSamples));

    sampleWidth = surface->lightmapDims[0];
    sampleHeight = surface->lightmapDims[1];

    normal = surface->plane.Normal();

    // debugging stuff - used to help visualize where texels are positioned in the level
#ifdef EXPORT_TEXELS_OBJ
    static int cnt = 0;
    FILE *f = fopen(Va("texels_%02d.obj", cnt++), "w");
    int indices = 0;
#endif

    // start walking through each texel
    for(i = 0; i < sampleHeight; i++)
    {
        for(j = 0; j < sampleWidth; j++)
        {
            // convert the texel into world-space coordinates.
            // this will be the origin in which a line will be traced from
            pos = surface->lightmapOrigin + normal +
                  (surface->lightmapSteps[0] * (float)j) +
                  (surface->lightmapSteps[1] * (float)i);

            // debugging stuff
#ifdef EXPORT_TEXELS_OBJ
            ExportTexelsToObjFile(f, pos, indices);
            indices += 8;
#endif

            // accumulate color samples
            colorSamples[i][j] += LightTexelSample(trace, pos, surface);

            // if nothing at all was traced and color is completely black
            // then this surface will not go through the extra rendering
            // step in rendering the lightmap
            if(colorSamples[i][j].UnitSq() != 0)
            {
                bShouldLookupTexture = true;
            }
        }
    }

#ifdef EXPORT_TEXELS_OBJ
    fclose(f);
#endif

    // SVE redraws the scene for lightmaps, so for optimizations,
    // tell the engine to ignore this surface if completely black
    if(bShouldLookupTexture == false)
    {
        surface->lightmapNum = -1;
        return;
    }
    else
    {
        int x = 0, y = 0;
        int width = surface->lightmapDims[0];
        int height = surface->lightmapDims[1];

        // now that we know the width and height of this block, see if we got
        // room for it in the light map texture. if not, then we must allocate
        // a new texture
        if(!MakeRoomForBlock(width, height, &x, &y, &surface->lightmapNum))
        {
            // allocate a new texture for this block
            lightmapWorker.LockMutex();
            NewTexture();
            lightmapWorker.UnlockMutex();

            if(!MakeRoomForBlock(width, height, &x, &y, &surface->lightmapNum))
            {
                Error("Lightmap allocation failed\n");
                return;
            }
        }

        // calculate texture coordinates
        for(i = 0; i < surface->numVerts; i++)
        {
            tDelta = surface->verts[i] - surface->bounds.min;
            surface->lightmapCoords[i * 2 + 0] =
                (tDelta.Dot(surface->textureCoords[0]) + x + 0.5f) / (float)textureWidth;
            surface->lightmapCoords[i * 2 + 1] =
                (tDelta.Dot(surface->textureCoords[1]) + y + 0.5f) / (float)textureHeight;
        }

        surface->lightmapOffs[0] = x;
        surface->lightmapOffs[1] = y;
    }

    lightmapWorker.LockMutex();
    currentTexture = textures[surface->lightmapNum];
    lightmapWorker.UnlockMutex();

    // store results to lightmap texture
    for(i = 0; i < sampleHeight; i++)
    {
        for(j = 0; j < sampleWidth; j++)
        {
            // get texture offset
            int offs = (((textureWidth * (i + surface->lightmapOffs[1])) +
                         surface->lightmapOffs[0]) * 3);

            // convert RGB to bytes
            rgb[0] = (byte)(colorSamples[i][j][0] * 255);
            rgb[1] = (byte)(colorSamples[i][j][1] * 255);
            rgb[2] = (byte)(colorSamples[i][j][2] * 255);

            currentTexture[offs + j * 3 + 0] = rgb[0];
            currentTexture[offs + j * 3 + 1] = rgb[1];
            currentTexture[offs + j * 3 + 2] = rgb[2];
        }
    }
}

//
// kexLightmapBuilder::LightSurface
//

void kexLightmapBuilder::LightSurface(const int surfid)
{
    static int processed = 0;
    float remaining;
    int numsurfs = surfaces.Length();

    // TODO: this should NOT happen, but apparently, it can randomly occur
    if(surfaces.Length() == 0)
    {
        return;
    }

    BuildSurfaceParams(surfaces[surfid]);
    TraceSurface(surfaces[surfid]);

    lightmapWorker.LockMutex();
    remaining = (float)processed / (float)numsurfs;
    processed++;

    printf("%i%c surfaces done\r", (int)(remaining * 100.0f), '%');
    lightmapWorker.UnlockMutex();
}

//
// kexLightmapBuilder::LightCellSample
//
// Traces a line from the cell's origin to the sunlight direction
// and against all nearby thing lights
//

kexVec3 kexLightmapBuilder::LightCellSample(const int gridid, kexTrace &trace,
        const kexVec3 &origin, const mapSubSector_t *sub)
{
    kexVec3 color;
    kexVec3 dir;
    mapSector_t *mapSector;
    float intensity;
    float radius;
    float dist;
    float colorAdd;
    thingLight_t *tl;
    kexVec3 lightOrigin;
    bool bInSkySector;
    kexVec3 org;

    mapSector = map->GetSectorFromSubSector(sub);
    bInSkySector = map->bSkySectors[mapSector - map->mapSectors];

    trace.Trace(origin, origin + (map->GetSunDirection() * 32768));

    // did we traced a ceiling surface with a sky texture?
    if(trace.fraction != 1 && trace.hitSurface != NULL)
    {
        if(trace.hitSurface->bSky && origin.z + gridSize[2] > mapSector->floorheight)
        {
            color = map->GetSunColor();
            // this cell is inside a sector with a sky texture and is also exposed to sunlight.
            // mark this cell as a sun type. cells of this type will simply sample the
            // sector's light level
            gridMap[gridid].sunShadow = 2;
            return color;
        }

        // if this cell is inside a sector with a sky texture but is NOT exposed to sunlight, then
        // mark this cell as a sun shade type. cells of this type will halve the sector's light level
        if(bInSkySector)
        {
            gridMap[gridid].sunShadow = 1;
        }
    }
    // if the cell is technically inside a sector with a sky but is not actually on an actual surface
    // then this cell is considered occluded
    else if(bInSkySector && !map->PointInsideSubSector(origin.x, origin.y, sub))
    {
        gridMap[gridid].sunShadow = 1;
    }

    // trace against all thing lights
    for(unsigned int i = 0; i < map->thingLights.Length(); i++)
    {
        tl = map->thingLights[i];

        lightOrigin.Set(tl->origin.x,
                        tl->origin.y,
                        !tl->bCeiling ?
                        (float)tl->sector->floorheight + 16 :
                        (float)tl->sector->ceilingheight - 16);

        radius = tl->radius;
        intensity = tl->intensity * 4;

        if(intensity < 1.0f)
        {
            intensity = 1.0f;
        }

        if(origin.DistanceSq(lightOrigin) > (radius*radius))
        {
            // not within range
            continue;
        }

        trace.Trace(origin, lightOrigin);

        if(trace.fraction != 1)
        {
            // something is occluding it
            continue;
        }

        dir = (lightOrigin - origin);
        dist = dir.Unit();

        dir.Normalize();

        colorAdd = (radius / (dist * dist)) * intensity;
        kexMath::Clamp(colorAdd, 0, 1);

        // accumulate results
        color = color.Lerp(tl->rgb, colorAdd);
        kexMath::Clamp(color, 0, 1);
    }

    org = origin;

    // if the cell is sticking out from the ground then at least
    // clamp the origin to the ground level so it can at least
    // have a chance to be sampled by the light
    if(origin.z + gridSize[2] > mapSector->floorheight)
    {
        org.z = (float)mapSector->floorheight + 2;
    }

    // trace against all light surfaces
    for(unsigned int i = 0; i < map->lightSurfaces.Length(); ++i)
    {
        kexLightSurface *surfaceLight = map->lightSurfaces[i];

        if(surfaceLight->TraceSurface(map, trace, NULL, org, &dist))
        {
            dist = (dist * (surfaceLight->Intensity() * 0.5f)) * 0.5f;
            kexMath::Clamp(dist, 0, 1);

            // accumulate results
            color = color.Lerp(surfaceLight->GetRGB(), dist);
            kexMath::Clamp(color, 0, 1);
        }
    }

    return color;
}

//
// kexLightmapBuilder::LightGrid
//

void kexLightmapBuilder::LightGrid(const int gridid)
{
    static int processed = 0;
    float remaining;
    int x, y, z;
    int mod;
    int secnum;
    bool bInRange;
    int gx = (int)gridBlock.x;
    int gy = (int)gridBlock.y;
    kexTrace trace;
    mapSubSector_t *ss;

    // convert grid id to xyz coordinates
    mod = gridid;
    z = mod / (gx * gy);
    mod -= z * (gx * gy);

    y = mod / gx;
    mod -= y * gx;

    x = mod;

    // get world-coordinates
    kexVec3 org(worldGrid.min[0] + x * gridSize[0],
                worldGrid.min[1] + y * gridSize[1],
                worldGrid.min[2] + z * gridSize[2]);

    ss = NULL;
    secnum = ((int)gridBlock.x * y) + x;

    // determine what sector this cell is in
    if(gridSectors[secnum] == NULL)
    {
        ss = map->PointInSubSector((int)org.x, (int)org.y);
        gridSectors[secnum] = ss;
    }
    else
    {
        ss = gridSectors[secnum];
    }

    trace.Init(*map);
    kexBBox bounds = gridBound + org;

    bInRange = false;

    // is this cell even inside the world?
    for(int i = 0; i < map->numSSects; ++i)
    {
        if(bounds.IntersectingBox(map->ssLeafBounds[i]))
        {
            bInRange = true;
            break;
        }
    }

    processed++;

    if(!bInRange)
    {
        // ignore if not in the world
        return;
    }

    // mark grid cell and accumulate color results
    gridMap[gridid].marked = 1;
    gridMap[gridid].color += LightCellSample(gridid, trace, org, ss);

    kexMath::Clamp(gridMap[gridid].color, 0, 1);

    lightmapWorker.LockMutex();
    remaining = (float)processed / (float)numLightGrids;

    printf("%i%c cells done\r", (int)(remaining * 100.0f), '%');
    lightmapWorker.UnlockMutex();
}

//
// kexLightmapBuilder::CreateLightmaps
//

void kexLightmapBuilder::CreateLightmaps(kexDoomMap &doomMap)
{
    map = &doomMap;

    printf("------------- Building light grid -------------\n");
    CreateLightGrid();

    printf("------------- Tracing surfaces -------------\n");
    lightmapWorker.RunThreads(surfaces.Length(), this, LightmapWorkerFunc);

    while(!lightmapWorker.FinishedAllJobs())
    {
        Delay(1000);
    }

    printf("Texels traced: %i\n\n", tracedTexels);
    lightmapWorker.Destroy();
}

//
// kexLightmapBuilder::CreateLightGrid
//

void kexLightmapBuilder::CreateLightGrid(void)
{
    int count;
    int numNodes;
    kexVec3 mins, maxs;

    // get the bounding box of the root BSP node
    numNodes = map->numNodes-1;
    if(numNodes < 0)
    {
        numNodes = 0;
    }

    worldGrid = map->nodeBounds[numNodes];

    // determine the size of the grid block
    for(int i = 0; i < 3; ++i)
    {
        mins[i] = gridSize[i] * kexMath::Ceil(worldGrid.min[i] / gridSize[i]);
        maxs[i] = gridSize[i] * kexMath::Floor(worldGrid.max[i] / gridSize[i]);
        gridBlock[i] = (maxs[i] - mins[i]) / gridSize[i] + 1;
    }

    worldGrid.min = mins;
    worldGrid.max = maxs;

    gridBound.min = -(gridSize * 0.5f);
    gridBound.max = (gridSize * 0.5f);

    // get the total number of grid cells
    count = (int)(gridBlock.x * gridBlock.y * gridBlock.z);
    numLightGrids = count;

    // allocate data
    gridMap = (gridMap_t*)Mem_Calloc(sizeof(gridMap_t) * count, hb_static);
    gridSectors = (mapSubSector_t**)Mem_Calloc(sizeof(mapSubSector_t*) *
                  (int)(gridBlock.x * gridBlock.y), hb_static);

    // process all grid cells
    lightmapWorker.RunThreads(count, this, LightGridWorkerFunc);

    while(!lightmapWorker.FinishedAllJobs())
    {
        Delay(1000);
    }

    printf("\nGrid cells: %i\n\n", count);
}

//
// kexLightmapBuilder::AddLightGridLump
//

void kexLightmapBuilder::AddLightGridLump(kexWadFile &wadFile)
{
    kexBinFile lumpFile;
    int lumpSize = 0;
    int bit;
    int bitmask;
    byte *data;

    lumpSize = 28 + numLightGrids;

    for(int i = 0; i < numLightGrids; ++i)
    {
        if(gridMap[i].marked)
        {
            lumpSize += 4;
        }
    }

    lumpSize += 512; // add some extra slop

    data = (byte*)Mem_Calloc(lumpSize, hb_static);
    lumpFile.SetBuffer(data);

    lumpFile.Write32(numLightGrids);
    lumpFile.Write16((short)worldGrid.min[0]);
    lumpFile.Write16((short)worldGrid.min[1]);
    lumpFile.Write16((short)worldGrid.min[2]);
    lumpFile.Write16((short)worldGrid.max[0]);
    lumpFile.Write16((short)worldGrid.max[1]);
    lumpFile.Write16((short)worldGrid.max[2]);
    lumpFile.Write16((short)gridSize.x);
    lumpFile.Write16((short)gridSize.y);
    lumpFile.Write16((short)gridSize.z);
    lumpFile.Write16((short)gridBlock.x);
    lumpFile.Write16((short)gridBlock.y);
    lumpFile.Write16((short)gridBlock.z);

    bit = 0;
    bitmask = 0;

    for(int i = 0; i < numLightGrids; ++i)
    {
#if 0
        bit |= (gridMap[i].marked << bitmask);

        if(++bitmask == 8)
        {
            lumpFile.Write8(bit);
            bit = 0;
            bitmask = 0;
        }
#else
        lumpFile.Write8(gridMap[i].marked);
#endif
    }

#if 0
    if(bit)
    {
        lumpFile.Write8(bit);
    }
#endif

    for(int i = 0; i < numLightGrids; ++i)
    {
        if(gridMap[i].marked)
        {
            lumpFile.Write8((byte)(gridMap[i].color[0] * 255.0f));
            lumpFile.Write8((byte)(gridMap[i].color[1] * 255.0f));
            lumpFile.Write8((byte)(gridMap[i].color[2] * 255.0f));
        }
    }

    bit = 0;
    bitmask = 0;

    for(int i = 0; i < numLightGrids; ++i)
    {
        if(gridMap[i].marked)
        {
#if 0
            bit |= (gridMap[i].sunShadow << bitmask);
            bitmask += 2;

            if(bitmask >= 8)
            {
                lumpFile.Write8(bit);
                bit = 0;
                bitmask = 0;
            }
#else
            lumpFile.Write8(gridMap[i].sunShadow);
#endif
        }
    }

#if 0
    if(bit)
    {
        lumpFile.Write8(bit);
    }
#endif

    wadFile.AddLump("LM_CELLS", lumpFile.BufferAt() - lumpFile.Buffer(), data);
}

//
// kexLightmapBuilder::CreateLightmapLump
//

void kexLightmapBuilder::AddLightmapLumps(kexWadFile &wadFile)
{
    int lumpSize = 0;
    unsigned int i;
    byte *data, *surfs, *txcrd, *lmaps;
    int offs;
    int size;
    int j;
    int coordOffsets;
    kexBinFile lumpFile;

    // try to guess the actual lump size
    lumpSize += ((textureWidth * textureHeight) * 3) * textures.Length();
    lumpSize += (12 * surfaces.Length());
    lumpSize += sizeof(kexVec3);
    lumpSize += 2048; // add some extra slop

    for(i = 0; i < surfaces.Length(); i++)
    {
        lumpSize += (surfaces[i]->numVerts * 2) * sizeof(float);
    }

    data = (byte*)Mem_Calloc(lumpSize, hb_static);
    lumpFile.SetBuffer(data);

    lumpFile.WriteVector(map->GetSunDirection());
    wadFile.AddLump("LM_SUN", sizeof(kexVec3), data);

    offs = lumpFile.BufferAt() - lumpFile.Buffer();
    surfs = data + offs;

    coordOffsets = 0;
    size = 0;

    // begin writing LM_SURFS lump
    for(i = 0; i < surfaces.Length(); i++)
    {
        lumpFile.Write16(surfaces[i]->type);
        lumpFile.Write16(surfaces[i]->typeIndex);
        lumpFile.Write16(surfaces[i]->lightmapNum);
        lumpFile.Write16(surfaces[i]->numVerts * 2);
        lumpFile.Write32(coordOffsets);

        size += 12;
        coordOffsets += (surfaces[i]->numVerts * 2);
    }

    offs = lumpFile.BufferAt() - lumpFile.Buffer();
    wadFile.AddLump("LM_SURFS", size, data);
    txcrd = data + offs;

    size = 0;

    // begin writing LM_TXCRD lump
    for(i = 0; i < surfaces.Length(); i++)
    {
        for(j = 0; j < surfaces[i]->numVerts * 2; j++)
        {
            lumpFile.WriteFloat(surfaces[i]->lightmapCoords[j]);
            size += 4;
        }
    }

    offs = (lumpFile.BufferAt() - lumpFile.Buffer()) - offs;
    wadFile.AddLump("LM_TXCRD", size, txcrd);
    lmaps = txcrd + offs;

    // begin writing LM_LMAPS lump
    lumpFile.Write32(textures.Length());
    lumpFile.Write32(textureWidth);
    lumpFile.Write32(textureHeight);

    size = 12;

    for(i = 0; i < textures.Length(); i++)
    {
        for(j = 0; j < (textureWidth * textureHeight) * 3; j++)
        {
            lumpFile.Write8(textures[i][j]);
            size++;
        }
    }

    offs = (lumpFile.BufferAt() - lumpFile.Buffer()) - offs;
    wadFile.AddLump("LM_LMAPS", size, lmaps);
}

//
// kexLightmapBuilder::WriteTexturesToTGA
//

void kexLightmapBuilder::WriteTexturesToTGA(void)
{
    kexBinFile file;

    for(unsigned int i = 0; i < textures.Length(); i++)
    {
        file.Create(Va("lightmap_%02d.tga", i));
        file.Write16(0);
        file.Write16(2);
        file.Write16(0);
        file.Write16(0);
        file.Write16(0);
        file.Write16(0);
        file.Write16(textureWidth);
        file.Write16(textureHeight);
        file.Write16(24);

        for(int j = 0; j < (textureWidth * textureHeight) * 3; j += 3)
        {
            file.Write8(textures[i][j+2]);
            file.Write8(textures[i][j+1]);
            file.Write8(textures[i][j+0]);
        }
        file.Close();
    }
}

//
// kexLightmapBuilder::ExportTexelsToObjFile
//

void kexLightmapBuilder::ExportTexelsToObjFile(FILE *f, const kexVec3 &org, int indices)
{
#define BLOCK(idx, offs) (org[idx] + offs)/256.0f
    fprintf(f, "o texel_%03d\n", indices);
    fprintf(f, "v %f %f %f\n", -BLOCK(1, -6), BLOCK(2, -6), -BLOCK(0, 6));
    fprintf(f, "v %f %f %f\n", -BLOCK(1, -6), BLOCK(2, 6), -BLOCK(0, 6));
    fprintf(f, "v %f %f %f\n", -BLOCK(1, -6), BLOCK(2, 6), -BLOCK(0, -6));
    fprintf(f, "v %f %f %f\n", -BLOCK(1, -6), BLOCK(2, -6), -BLOCK(0, -6));
    fprintf(f, "v %f %f %f\n", -BLOCK(1, 6), BLOCK(2, -6), -BLOCK(0, 6));
    fprintf(f, "v %f %f %f\n", -BLOCK(1, 6), BLOCK(2, 6), -BLOCK(0, 6));
    fprintf(f, "v %f %f %f\n", -BLOCK(1, 6), BLOCK(2, 6), -BLOCK(0, -6));
    fprintf(f, "v %f %f %f\n", -BLOCK(1, 6), BLOCK(2, -6), -BLOCK(0, -6));
    fprintf(f, "f %i %i %i %i\n", indices+1, indices+2, indices+3, indices+4);
    fprintf(f, "f %i %i %i %i\n", indices+5, indices+8, indices+7, indices+6);
    fprintf(f, "f %i %i %i %i\n", indices+1, indices+5, indices+6, indices+2);
    fprintf(f, "f %i %i %i %i\n", indices+2, indices+6, indices+7, indices+3);
    fprintf(f, "f %i %i %i %i\n", indices+3, indices+7, indices+8, indices+4);
    fprintf(f, "f %i %i %i %i\n\n", indices+5, indices+1, indices+4, indices+8);
#undef BLOCK
}

//
// kexLightmapBuilder::WriteBlock
//

void kexLightmapBuilder::WriteBlock(FILE *f, const int i, const kexVec3 &org, int indices, kexBBox &box)
{
#define BLOCK(idx, offs) (org[idx] + box[offs][idx])/256.0f
    fprintf(f, "o texel_%03d\n", i);
    fprintf(f, "v %f %f %f\n", -BLOCK(1, 0), BLOCK(2, 0), -BLOCK(0, 1));
    fprintf(f, "v %f %f %f\n", -BLOCK(1, 0), BLOCK(2, 1), -BLOCK(0, 1));
    fprintf(f, "v %f %f %f\n", -BLOCK(1, 0), BLOCK(2, 1), -BLOCK(0, 0));
    fprintf(f, "v %f %f %f\n", -BLOCK(1, 0), BLOCK(2, 0), -BLOCK(0, 0));
    fprintf(f, "v %f %f %f\n", -BLOCK(1, 1), BLOCK(2, 0), -BLOCK(0, 1));
    fprintf(f, "v %f %f %f\n", -BLOCK(1, 1), BLOCK(2, 1), -BLOCK(0, 1));
    fprintf(f, "v %f %f %f\n", -BLOCK(1, 1), BLOCK(2, 1), -BLOCK(0, 0));
    fprintf(f, "v %f %f %f\n", -BLOCK(1, 1), BLOCK(2, 0), -BLOCK(0, 0));
    fprintf(f, "f %i %i %i %i\n", indices+1, indices+2, indices+3, indices+4);
    fprintf(f, "f %i %i %i %i\n", indices+5, indices+8, indices+7, indices+6);
    fprintf(f, "f %i %i %i %i\n", indices+1, indices+5, indices+6, indices+2);
    fprintf(f, "f %i %i %i %i\n", indices+2, indices+6, indices+7, indices+3);
    fprintf(f, "f %i %i %i %i\n", indices+3, indices+7, indices+8, indices+4);
    fprintf(f, "f %i %i %i %i\n\n", indices+5, indices+1, indices+4, indices+8);
#undef BLOCK
}
