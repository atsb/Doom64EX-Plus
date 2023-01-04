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
// DESCRIPTION: General class module for handling ray tracing of the
//              world geometry. Ideally, all of this needs to be revisited...
//
//-----------------------------------------------------------------------------

#include "common.h"
#include "mapData.h"
#include "trace.h"

//
// kexTrace::kexTrace
//

kexTrace::kexTrace(void)
{
    this->map = NULL;
}

//
// kexTrace::~kexTrace
//

kexTrace::~kexTrace(void)
{
}

//
// kexTrace::Init
//

void kexTrace::Init(kexDoomMap &doomMap)
{
    map = &doomMap;
}

//
// kexTrace::Trace
//

void kexTrace::Trace(const kexVec3 &startVec, const kexVec3 &endVec)
{
    start = startVec;
    end = endVec;
    dir = (end - start).Normalize();
    hitNormal.Clear();
    hitVector.Clear();
    hitSurface = NULL;
    fraction = 1;

    if(map == NULL)
    {
        return;
    }

    TraceBSPNode(map->numNodes - 1);
}

//
// kexTrace::TraceSurface
//

void kexTrace::TraceSurface(surface_t *surface)
{
    kexPlane *plane;
    kexVec3 hit;
    kexVec3 edge1;
    kexVec3 edge2;
    kexVec3 normal;
    float d1;
    float d2;
    float d;
    float frac;
    int i;
    kexPluecker r;

    if(surface == NULL)
    {
        return;
    }

    plane = &surface->plane;

    d1 = plane->Distance(start) - plane->d;
    d2 = plane->Distance(end) - plane->d;

    if(d1 <= d2 || d1 < 0 || d2 > 0)
    {
        // trace is either completely in front or behind the plane
        return;
    }

    frac = (d1 / (d1 - d2));

    if(frac > 1 || frac < 0)
    {
        // not a valid contact
        return;
    }

    if(frac >= fraction)
    {
        // farther than the current contact
        return;
    }

    hit = start.Lerp(end, frac);
    normal = plane->Normal();

    r.SetRay(start, dir);

    // segs are always made up of 4 vertices, so its safe to assume 4 edges here
    if(surface->type >= ST_MIDDLESEG && surface->type <= ST_LOWERSEG)
    {
        kexPluecker p1;
        kexPluecker p2;
        kexPluecker p3;
        kexPluecker p4;
        byte sideBits = 0;

        p1.SetLine(surface->verts[2], surface->verts[3]); // top edge
        p2.SetLine(surface->verts[1], surface->verts[0]); // bottom edge
        p3.SetLine(surface->verts[3], surface->verts[1]); // right edge
        p4.SetLine(surface->verts[0], surface->verts[2]); // left edge

        // this sucks so much..... I am surprised this even works at all
        d = r.InnerProduct(p1)-0.001f;
        sideBits |= (FLOATSIGNBIT(d) << 0);
        d = r.InnerProduct(p2)-0.001f;
        sideBits |= (FLOATSIGNBIT(d) << 1);
        d = r.InnerProduct(p3)-0.001f;
        sideBits |= (FLOATSIGNBIT(d) << 2);
        d = r.InnerProduct(p4)-0.001f;
        sideBits |= (FLOATSIGNBIT(d) << 3);

        if(sideBits != 0xF)
        {
            return;
        }
    }
    else if(surface->type == ST_FLOOR || surface->type == ST_CEILING)
    {
        kexPluecker p;
        kexVec3 v1;
        kexVec3 v2;

        for(i = 0; i < surface->numVerts; i++)
        {
            v1 = surface->verts[i];
            v2 = surface->verts[(i+1)%surface->numVerts];

            p.SetLine(v2, v1);

            if(r.InnerProduct(p) > 0.01f)
            {
                return;
            }
        }
    }

    hitNormal = normal;
    hitVector = hit;
    hitSurface = surface;
    fraction = frac;
}

//
// kexTrace::TraceSubSector
//

void kexTrace::TraceSubSector(int num)
{
    mapSubSector_t *sub;
    int i;
    int j;

    sub = &map->mapSSects[num];

    if(!map->ssLeafBounds[num].LineIntersect(start, end))
    {
        return;
    }

    // test line segments
    for(i = 0; i < sub->numsegs; i++)
    {
        int segnum = sub->firstseg + i;

        for(j = 0; j < 3; j++)
        {
            if(j == 0)
            {
                int linenum = map->mapSegs[segnum].linedef;

                if(linenum != NO_LINE_INDEX && map->mapLines[linenum].flags &
                        (ML_TWOSIDED|ML_TRANSPARENT1|ML_TRANSPARENT2))
                {
                    // don't trace transparent 2-sided lines
                    continue;
                }
            }
            TraceSurface(map->segSurfaces[j][segnum]);
        }
    }

    // test subsector leafs
    for(j = 0; j < 2; j++)
    {
        TraceSurface(map->leafSurfaces[j][num]);
    }
}

//
// kexTrace::TraceBSPNode
//

void kexTrace::TraceBSPNode(int num)
{
    mapNode_t *node;
    kexVec3 dp1;
    kexVec3 dp2;
    float d;
    byte side;

    if(num & NF_SUBSECTOR)
    {
        TraceSubSector(num & (~NF_SUBSECTOR));
        return;
    }

    if(!map->nodeBounds[num & (~NF_SUBSECTOR)].LineIntersect(start, end))
    {
        return;
    }

    node = &map->nodes[num];

    kexVec3 pt1(F(node->x << 16), F(node->y << 16), 0);
    kexVec3 pt2(F(node->dx << 16), F(node->dy << 16), 0);

    dp1 = pt1 - start;
    dp2 = (pt2 + pt1) - start;
    d = dp1.Cross(dp2).z;

    side = FLOATSIGNBIT(d);

    TraceBSPNode(node->children[side ^ 1]);

    dp1 = pt1 - end;
    dp2 = (pt2 + pt1) - end;
    d = dp1.Cross(dp2).z;

    // don't trace if both ends of the ray are on the same side
    if(side != FLOATSIGNBIT(d))
    {
        TraceBSPNode(node->children[side]);
    }
}
