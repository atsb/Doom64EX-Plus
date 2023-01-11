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
// DESCRIPTION: General doom map utilities and data preperation
//
//-----------------------------------------------------------------------------

#include "common.h"
#include "wad.h"
#include "kexlib/parser.h"
#include "mapData.h"
#include "lightSurface.h"

const kexVec3 kexDoomMap::defaultSunColor(1, 1, 1);
const kexVec3 kexDoomMap::defaultSunDirection(0.45f, 0.3f, 0.9f);

//
// kexDoomMap::kexDoomMap
//

kexDoomMap::kexDoomMap(void)
{
    this->mapLines          = NULL;
    this->mapVerts          = NULL;
    this->mapSides          = NULL;
    this->mapSectors        = NULL;
    this->mapSegs           = NULL;
    this->mapSSects         = NULL;
    this->nodes             = NULL;
    this->leafs             = NULL;
    this->segLeafLookup     = NULL;
    this->ssLeafLookup      = NULL;
    this->ssLeafCount       = NULL;
    this->segSurfaces[0]    = NULL;
    this->segSurfaces[1]    = NULL;
    this->segSurfaces[2]    = NULL;
    this->leafSurfaces[0]   = NULL;
    this->leafSurfaces[1]   = NULL;
    this->vertexes          = NULL;
    this->mapPVS            = NULL;
    this->mapDef            = NULL;

    this->numLeafs      = 0;
    this->numLines      = 0;
    this->numVerts      = 0;
    this->numSides      = 0;
    this->numSectors    = 0;
    this->numSegs       = 0;
    this->numSSects     = 0;
    this->numNodes      = 0;
    this->numVertexes   = 0;
}

//
// kexDoomMap::~kexDoomMap
//

kexDoomMap::~kexDoomMap(void)
{
}

//
// kexDoomMap::BuildMapFromWad
//

void kexDoomMap::BuildMapFromWad(kexWadFile &wadFile)
{
    wadFile.GetMapLump<mapThing_t>(ML_THINGS, &mapThings, &numThings);
    wadFile.GetMapLump<mapVertex_t>(ML_VERTEXES, &mapVerts, &numVerts);
    wadFile.GetMapLump<mapLineDef_t>(ML_LINEDEFS, &mapLines, &numLines);
    wadFile.GetMapLump<mapSideDef_t>(ML_SIDEDEFS, &mapSides, &numSides);
    wadFile.GetMapLump<mapSector_t>(ML_SECTORS, &mapSectors, &numSectors);

    wadFile.GetGLMapLump<glSeg_t>(ML_GL_SEGS, &mapSegs, &numSegs);
    wadFile.GetGLMapLump<mapSubSector_t>(ML_GL_SSECT, &mapSSects, &numSSects);
    wadFile.GetGLMapLump<mapNode_t>(ML_GL_NODES, &nodes, &numNodes);
    wadFile.GetGLMapLump<byte>(ML_GL_PVS, &mapPVS, 0);

    if(mapSegs == NULL)
    {
        Error("kexDoomMap::BuildMapFromWad: SEGS lump not found\n");
        return;
    }
    if(mapSSects == NULL)
    {
        Error("kexDoomMap::BuildMapFromWad: SSECTORS lump not found\n");
        return;
    }
    if(nodes == NULL)
    {
        Error("kexDoomMap::BuildMapFromWad: NODES lump not found\n");
        return;
    }

    for(unsigned int i = 0; i < mapDefs.Length(); ++i)
    {
        if(mapDefs[i].map == wadFile.currentmap)
        {
            mapDef = &mapDefs[i];
            break;
        }
    }

    printf("------------- Level Info -------------\n");
    printf("Vertices: %i\n", numVerts);
    printf("Segments: %i\n", numSegs);
    printf("Subsectors: %i\n", numSSects);

    BuildVertexes(wadFile);
    BuildNodeBounds();
    BuildLeafs();
    BuildPVS();
    CheckSkySectors();
}

//
// kexDoomMap::CheckSkySectors
//

void kexDoomMap::CheckSkySectors(void)
{
    char name[9];

    bSkySectors = (bool*)Mem_Calloc(sizeof(bool) * numSectors, hb_static);
    bSSectsVisibleToSky = (bool*)Mem_Calloc(sizeof(bool) * numSSects, hb_static);

    for(int i = 0; i < numSectors; ++i)
    {
        if(mapDef->sunIgnoreTag != 0 && mapSectors[i].tag == mapDef->sunIgnoreTag)
        {
            continue;
        }

        strncpy(name, mapSectors[i].ceilingpic, 8);
        name[8] = 0;

        if(!strncmp(name, "F_SKY001", 8))
        {
            bSkySectors[i] = true;
        }
    }

    // try to early out by quickly checking which subsector can potentially
    // see a sky sector
    for(int i = 0; i < numSSects; ++i)
    {
        for(int j = 0; j < numSSects; ++j)
        {
            mapSector_t *sec = GetSectorFromSubSector(&mapSSects[j]);

            if(bSkySectors[sec - mapSectors] == false)
            {
                continue;
            }

            if(CheckPVS(&mapSSects[i], &mapSSects[j]))
            {
                bSSectsVisibleToSky[i] = true;
                break;
            }
        }
    }
}

//
// kexDoomMap::BuildPVS
//

void kexDoomMap::BuildPVS(void)
{
    // don't do anything if already loaded
    if(mapPVS != NULL)
    {
        return;
    }

    int len = ((numSSects + 7) / 8) * numSSects;
    mapPVS = (byte*)Mem_Malloc(len, hb_static);
    memset(mapPVS, 0xff, len);
}

//
// kexDoomMap::CheckPVS
//

bool kexDoomMap::CheckPVS(mapSubSector_t *s1, mapSubSector_t *s2)
{
    byte *vis;
    int n1, n2;

    n1 = s1 - mapSSects;
    n2 = s2 - mapSSects;

    vis = &mapPVS[(((numSSects + 7) / 8) * n1)];

    return ((vis[n2 >> 3] & (1 << (n2 & 7))) != 0);
}

//
// kexDoomMap::BuildVertexes
//

void kexDoomMap::BuildVertexes(kexWadFile &wadFile)
{
    byte *data;
    int count;
    glVert_t *verts;
    lump_t *lump;

    if(!(lump = wadFile.GetGLMapLump(static_cast<glMapLumps_t>(ML_GL_VERTS))))
    {
        Error("kexDoomMap::BuildVertexes: GL_VERTS lump not found\n");
        return;
    }

    data = wadFile.GetLumpData(lump);

    if(*((int*)data) != gNd2)
    {
        Error("kexDoomMap::BuildVertexes: GL_VERTS must be version 2 only");
        return;
    }

    verts = (glVert_t*)(data + GL_VERT_OFFSET);
    count = (lump->size - GL_VERT_OFFSET) / sizeof(glVert_t);

    numVertexes = numVerts + count;
    vertexes = (vertex_t*)Mem_Calloc(sizeof(vertex_t) * numVertexes, hb_static);

    for(int i = 0; i < numVerts; i++)
    {
        vertexes[i].x = F(mapVerts[i].x << 16);
        vertexes[i].y = F(mapVerts[i].y << 16);
    }

    for(int i = 0; i < count; i++)
    {
        vertexes[numVerts + i].x = F(verts[i].x);
        vertexes[numVerts + i].y = F(verts[i].y);
    }
}

//
// kexDoomMap::BuildNodeBounds
//

void kexDoomMap::BuildNodeBounds(void)
{
    int     i;
    int     j;
    kexVec3 point;
    float   high = -M_INFINITY;
    float   low = M_INFINITY;

    nodeBounds = (kexBBox*)Mem_Calloc(sizeof(kexBBox) * numNodes, hb_static);

    for(i = 0; i < numSectors; ++i)
    {
        if(mapSectors[i].ceilingheight > high)
        {
            high = mapSectors[i].ceilingheight;
        }
        if(mapSectors[i].floorheight < low)
        {
            low = mapSectors[i].floorheight;
        }
    }

    for(i = 0; i < numNodes; ++i)
    {
        nodeBounds[i].Clear();

        for(j = 0; j < 2; ++j)
        {
            point.Set(nodes[i].bbox[j][BOXLEFT], nodes[i].bbox[j][BOXBOTTOM], low);
            nodeBounds[i].AddPoint(point);
            point.Set(nodes[i].bbox[j][BOXRIGHT], nodes[i].bbox[j][BOXTOP], high);
            nodeBounds[i].AddPoint(point);
        }
    }
}

//
// kexDoomMap::BuildLeafs
//

void kexDoomMap::BuildLeafs(void)
{
    mapSubSector_t  *ss;
    leaf_t          *lf;
    int             i;
    int             j;
    kexVec3         point;
    mapSector_t     *sector;
    int             count;

    leafs = (leaf_t*)Mem_Calloc(sizeof(leaf_t*) * numSegs * 2, hb_static);
    numLeafs = numSSects;

    ss = mapSSects;

    segLeafLookup = (int*)Mem_Calloc(sizeof(int) * numSegs, hb_static);
    ssLeafLookup = (int*)Mem_Calloc(sizeof(int) * numSSects, hb_static);
    ssLeafCount = (int*)Mem_Calloc(sizeof(int) * numSSects, hb_static);
    ssLeafBounds = (kexBBox*)Mem_Calloc(sizeof(kexBBox) * numSSects, hb_static);

    count = 0;

    for(i = 0; i < numSSects; ++i, ++ss)
    {
        ssLeafCount[i] = ss->numsegs;
        ssLeafLookup[i] = ss->firstseg;

        ssLeafBounds[i].Clear();
        sector = GetSectorFromSubSector(ss);

        if(ss->numsegs)
        {
            for(j = 0; j < ss->numsegs; ++j)
            {
                glSeg_t *seg = &mapSegs[ss->firstseg + j];
                lf = &leafs[count++];

                segLeafLookup[ss->firstseg + j] = i;

                lf->vertex = GetSegVertex(seg->v1);
                lf->seg = seg;

                point.Set(lf->vertex->x, lf->vertex->y, sector->floorheight);
                ssLeafBounds[i].AddPoint(point);

                point.z = sector->ceilingheight;
                ssLeafBounds[i].AddPoint(point);
            }
        }
    }
}

//
// kexDoomMap::GetSegVertex
//

vertex_t *kexDoomMap::GetSegVertex(int index)
{
    if(index & 0x8000)
    {
        index = (index & 0x7FFF) + numVerts;
    }

    return &vertexes[index];
}

//
// kexDoomMap::GetSideDef
//

mapSideDef_t *kexDoomMap::GetSideDef(const glSeg_t *seg)
{
    mapLineDef_t *line;

    if(seg->linedef == NO_LINE_INDEX)
    {
        // skip minisegs
        return NULL;
    }

    line = &mapLines[seg->linedef];
    return &mapSides[line->sidenum[seg->side]];
}

//
// kexDoomMap::GetFrontSector
//

mapSector_t *kexDoomMap::GetFrontSector(const glSeg_t *seg)
{
    mapSideDef_t *side = GetSideDef(seg);

    if(side == NULL)
    {
        return NULL;
    }

    return &mapSectors[side->sector];
}

//
// kexDoomMap::GetBackSector
//

mapSector_t *kexDoomMap::GetBackSector(const glSeg_t *seg)
{
    mapLineDef_t *line;

    if(seg->linedef == NO_LINE_INDEX)
    {
        // skip minisegs
        return NULL;
    }

    line = &mapLines[seg->linedef];

    if(line->flags & ML_TWOSIDED)
    {
        mapSideDef_t *backSide = &mapSides[line->sidenum[seg->side^1]];
        return &mapSectors[backSide->sector];
    }

    return NULL;
}

//
// kexDoomMap::GetSectorFromSubSector
//

mapSector_t *kexDoomMap::GetSectorFromSubSector(const mapSubSector_t *sub)
{
    mapSector_t *sector = NULL;

    // try to find a sector that the subsector belongs to
    for(int i = 0; i < sub->numsegs; i++)
    {
        glSeg_t *seg = &mapSegs[sub->firstseg + i];
        if(seg->side != NO_SIDE_INDEX)
        {
            sector = GetFrontSector(seg);
            break;
        }
    }

    return sector;
}

//
// kexDoomMap::PointInSubSector
//

mapSubSector_t *kexDoomMap::PointInSubSector(const int x, const int y)
{
    mapNode_t   *node;
    int         side;
    int         nodenum;
    kexVec3     dp1;
    kexVec3     dp2;
    float       d;

    // single subsector is a special case
    if(!numNodes)
    {
        return &mapSSects[0];
    }

    nodenum = numNodes - 1;

    while(!(nodenum & NF_SUBSECTOR) )
    {
        node = &nodes[nodenum];

        kexVec3 pt1(F(node->x << 16), F(node->y << 16), 0);
        kexVec3 pt2(F(node->dx << 16), F(node->dy << 16), 0);
        kexVec3 pos(F(x << 16), F(y << 16), 0);

        dp1 = pt1 - pos;
        dp2 = (pt2 + pt1) - pos;
        d = dp1.Cross(dp2).z;

        side = FLOATSIGNBIT(d);

        nodenum = node->children[side ^ 1];
    }

    return &mapSSects[nodenum & ~NF_SUBSECTOR];
}

//
// kexDoomMap::PointInsideSubSector
//

bool kexDoomMap::PointInsideSubSector(const float x, const float y, const mapSubSector_t *sub)
{
    surface_t *surf;
    int i;
    kexVec2 p(x, y);
    kexVec2 dp1, dp2;
    kexVec2 pt1, pt2;

    surf = leafSurfaces[0][sub - mapSSects];

    // check to see if the point is inside the subsector leaf
    for(i = 0; i < surf->numVerts; i++)
    {
        pt1 = surf->verts[i].ToVec2();
        pt2 = surf->verts[(i+1)%surf->numVerts].ToVec2();

        dp1 = pt1 - p;
        dp2 = pt2 - p;

        if(dp1.CrossScalar(dp2) < 0)
        {
            continue;
        }

        // this point is outside the subsector leaf
        return false;
    }

    return true;
}

//
// kexDoomMap::LineIntersectSubSector
//

bool kexDoomMap::LineIntersectSubSector(const kexVec3 &start, const kexVec3 &end,
                                        const mapSubSector_t *sub, kexVec2 &out)
{
    surface_t *surf;
    kexVec2 p1, p2;
    kexVec2 s1, s2;
    kexVec2 pt;
    kexVec2 v;
    float d, u;
    float newX;
    float ab;
    int i;

    surf = leafSurfaces[0][sub - mapSSects];
    p1 = start.ToVec2();
    p2 = end.ToVec2();

    for(i = 0; i < surf->numVerts; i++)
    {
        s1 = surf->verts[i].ToVec2();
        s2 = surf->verts[(i+1)%surf->numVerts].ToVec2();

        if((p1 == p2) || (s1 == s2))
        {
            // zero length
            continue;
        }

        if((p1 == s1) || (p2 == s1) || (p1 == s2) || (p2 == s2))
        {
            // shares end point
            continue;
        }

        // translate to origin
        pt = p2 - p1;
        s1 -= p1;
        s2 -= p1;

        // normalize
        u = pt.UnitSq();
        d = kexMath::InvSqrt(u);
        v = (pt * d);

        // rotate points s1 and s2 so they're on the positive x axis
        newX = s1.Dot(v);
        s1.y = s1.CrossScalar(v);
        s1.x = newX;

        newX = s2.Dot(v);
        s2.y = s2.CrossScalar(v);
        s2.x = newX;

        if((s1.y < 0 && s2.y < 0) || (s1.y >= 0 && s2.y >= 0))
        {
            // s1 and s2 didn't cross
            continue;
        }

        ab = s2.x + (s1.x - s2.x) * s2.y / (s2.y - s1.y);

        if(ab < 0 || ab > (u * d))
        {
            // s1 and s2 crosses but outside of points p1 and p2
            continue;
        }

        // intersected
        out = p1 + (v * ab);
        return true;
    }

    return false;
}

//
// kexDoomMap::GetSunColor
//

const kexVec3 &kexDoomMap::GetSunColor(void) const
{
    if(mapDef != NULL)
    {
        return mapDef->sunColor;
    }

    return defaultSunColor;
}

//
// kexDoomMap::GetSunDirection
//

const kexVec3 &kexDoomMap::GetSunDirection(void) const
{
    if(mapDef != NULL)
    {
        return mapDef->sunDir;
    }

    return defaultSunDirection;
}

//
// kexDoomMap::ParseConfigFile
//

void kexDoomMap::ParseConfigFile(const char *file)
{
    kexLexer *lexer;

    if(!(lexer = parser->Open(file)))
    {
        Error("kexDoomMap::ParseConfigFile: %s not found\n", file);
        return;
    }

    while(lexer->CheckState())
    {
        lexer->Find();

        // check for mapdef block
        if(lexer->Matches("mapdef"))
        {
            mapDef_t mapDef;

            mapDef.map = -1;
            mapDef.sunIgnoreTag = 0;

            lexer->ExpectNextToken(TK_LBRACK);
            lexer->Find();

            while(lexer->TokenType() != TK_RBRACK)
            {
                if(lexer->Matches("map"))
                {
                    mapDef.map = lexer->GetNumber();
                }
                else if(lexer->Matches("sun_ignore_tag"))
                {
                    mapDef.sunIgnoreTag = lexer->GetNumber();
                }
                else if(lexer->Matches("sun_direction"))
                {
                    mapDef.sunDir = lexer->GetVectorString3();
                }
                else if(lexer->Matches("sun_color"))
                {
                    mapDef.sunColor = lexer->GetVectorString3();
                    mapDef.sunColor /= 255.0f;
                }

                lexer->Find();
            }

            mapDefs.Push(mapDef);
        }

        // check for lightdef block
        if(lexer->Matches("lightdef"))
        {
            lightDef_t lightDef;

            lightDef.doomednum = -1;
            lightDef.height = 0;
            lightDef.intensity = 2;
            lightDef.falloff = 1;
            lightDef.bCeiling = false;

            lexer->ExpectNextToken(TK_LBRACK);
            lexer->Find();

            while(lexer->TokenType() != TK_RBRACK)
            {
                if(lexer->Matches("doomednum"))
                {
                    lightDef.doomednum = lexer->GetNumber();
                }
                else if(lexer->Matches("rgb"))
                {
                    lightDef.rgb = lexer->GetVectorString3();
                    lightDef.rgb /= 255.0f;
                }
                else if(lexer->Matches("height"))
                {
                    lightDef.height = (float)lexer->GetFloat();
                }
                else if(lexer->Matches("radius"))
                {
                    lightDef.radius = (float)lexer->GetFloat();
                }
                else if(lexer->Matches("intensity"))
                {
                    lightDef.intensity = (float)lexer->GetFloat();
                }
                else if(lexer->Matches("falloff"))
                {
                    lightDef.falloff = (float)lexer->GetFloat();
                }
                else if(lexer->Matches("ceiling"))
                {
                    lightDef.bCeiling = true;
                }

                lexer->Find();
            }

            lightDefs.Push(lightDef);
        }

        if(lexer->Matches("surfaceLight"))
        {
            surfaceLightDef_t surfaceLight;

            surfaceLight.tag = 0;
            surfaceLight.outerCone = 1.0f;
            surfaceLight.innerCone = 0;
            surfaceLight.falloff = 1.0f;
            surfaceLight.intensity = 1.0f;
            surfaceLight.distance = 32.0f;
            surfaceLight.bIgnoreCeiling = false;
            surfaceLight.bIgnoreFloor = false;
            surfaceLight.bNoCenterPoint = false;

            lexer->ExpectNextToken(TK_LBRACK);
            lexer->Find();

            while(lexer->TokenType() != TK_RBRACK)
            {
                if(lexer->Matches("tag"))
                {
                    surfaceLight.tag = lexer->GetNumber();
                }
                else if(lexer->Matches("rgb"))
                {
                    surfaceLight.rgb = lexer->GetVectorString3();
                    surfaceLight.rgb /= 255.0f;
                }
                else if(lexer->Matches("cone_outer"))
                {
                    surfaceLight.outerCone = (float)lexer->GetFloat() / 180.0f;
                }
                else if(lexer->Matches("cone_inner"))
                {
                    surfaceLight.innerCone = (float)lexer->GetFloat() / 180.0f;
                }
                else if(lexer->Matches("falloff"))
                {
                    surfaceLight.falloff = (float)lexer->GetFloat();
                }
                else if(lexer->Matches("intensity"))
                {
                    surfaceLight.intensity = (float)lexer->GetFloat();
                }
                else if(lexer->Matches("distance"))
                {
                    surfaceLight.distance = (float)lexer->GetFloat();
                }
                else if(lexer->Matches("bIgnoreCeiling"))
                {
                    surfaceLight.bIgnoreCeiling = true;
                }
                else if(lexer->Matches("bIgnoreFloor"))
                {
                    surfaceLight.bIgnoreFloor = true;
                }
                else if(lexer->Matches("bNoCenterPoint"))
                {
                    surfaceLight.bNoCenterPoint = true;
                }

                lexer->Find();
            }

            surfaceLightDefs.Push(surfaceLight);
        }
    }

    // we're done with the file
    parser->Close();
}

//
// kexDoomMap::CreateLights
//

void kexDoomMap::CreateLights(void)
{
    mapThing_t *thing;
    thingLight_t *thingLight;
    unsigned int j;
    int numSurfLights;
    kexVec2 pt;

    //
    // add lights from thing sources
    //
    for(int i = 0; i < numThings; ++i)
    {
        lightDef_t *lightDef = NULL;

        thing = &mapThings[i];

        for(j = 0; j < lightDefs.Length(); ++j)
        {
            if(thing->type == lightDefs[j].doomednum)
            {
                lightDef = &lightDefs[j];
                break;
            }
        }

        if(!lightDef)
        {
            continue;
        }

        if(lightDef->radius >= 0)
        {
            // ignore if all skills aren't set
            if(!(thing->options & 7))
            {
                continue;
            }
        }

        thingLight = new thingLight_t;

        thingLight->mapThing    = thing;
        thingLight->rgb         = lightDef->rgb;
        thingLight->intensity   = lightDef->intensity;
        thingLight->falloff     = lightDef->falloff;
        thingLight->radius      = lightDef->radius >= 0 ? lightDef->radius : thing->angle;
        thingLight->height      = lightDef->height;
        thingLight->bCeiling    = lightDef->bCeiling;
        thingLight->ssect       = PointInSubSector(thing->x, thing->y);
        thingLight->sector      = GetSectorFromSubSector(thingLight->ssect);

        thingLight->origin.Set(thing->x, thing->y);
        thingLights.Push(thingLight);
    }

    printf("Thing lights: %i\n", thingLights.Length());

    numSurfLights = 0;

    //
    // add surface lights
    //
    for(j = 0; j < surfaces.Length(); ++j)
    {
        surface_t *surface = surfaces[j];

        for(unsigned int k = 0; k < surfaceLightDefs.Length(); ++k)
        {
            surfaceLightDef_t *surfaceLightDef = &surfaceLightDefs[k];

            if(surface->type >= ST_MIDDLESEG && surface->type <= ST_LOWERSEG)
            {
                glSeg_t *seg = (glSeg_t*)surface->data;

                if(mapLines[seg->linedef].tag == surfaceLightDef->tag)
                {
                    kexLightSurface *lightSurface = new kexLightSurface;

                    lightSurface->Init(*surfaceLightDef, surface, true, false);
                    lightSurface->CreateCenterOrigin();
                    lightSurfaces.Push(lightSurface);
                    numSurfLights++;
                }
            }
            else
            {
                mapSubSector_t *sub = surface->subSector;
                mapSector_t *sector = GetSectorFromSubSector(sub);

                if(!sector || surface->numVerts <= 0)
                {
                    // eh....
                    continue;
                }

                if(surface->type == ST_CEILING && surfaceLightDef->bIgnoreCeiling)
                {
                    continue;
                }

                if(surface->type == ST_FLOOR && surfaceLightDef->bIgnoreFloor)
                {
                    continue;
                }

                if(sector->tag == surfaceLightDef->tag)
                {
                    kexLightSurface *lightSurface = new kexLightSurface;

                    lightSurface->Init(*surfaceLightDef, surface, false, surfaceLightDef->bNoCenterPoint);
                    lightSurface->Subdivide(16);
                    lightSurfaces.Push(lightSurface);
                    numSurfLights++;
                }
            }
        }
    }

    printf("Surface lights: %i\n", numSurfLights);
}

//
// kexDoomMap::CleanupThingLights
//

void kexDoomMap::CleanupThingLights(void)
{
    for(unsigned int i = 0; i < thingLights.Length(); i++)
    {
        delete thingLights[i];
    }
}
