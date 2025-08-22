// Emacs style mode select   -*- C -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 1993-1997 Id Software, Inc.
// Copyright(C) 2007-2012 Samuel Villarreal
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
//-----------------------------------------------------------------------------
//
// DESCRIPTION:
//    Movement/collision utility functions,
//    as used by function in p_map.c.
//    BLOCKMAP Iterator functions,
//    and some PIT_* functions to use for iteration.
//
//-----------------------------------------------------------------------------

#include <limits.h>
#include "m_misc.h"
#include "m_fixed.h"
#include "doomdef.h"
#include "p_local.h"
#include "doomstat.h"
#include "i_system.h"
#include "i_swap.h"
#include "r_main.h"

// atsb: Reverse Engineered

intercept_t intercepts[MAXINTERCEPTS];
intercept_t* intercept_p;

divline_t trace;
int ptflags;

//
// P_AproxDistance
//
fixed_t P_AproxDistance(fixed_t dx, fixed_t dy)
{
    dx = D_abs(dx);
    dy = D_abs(dy);
    if (dx < dy) 
        return dx + dy - (dx >> 1);
    return dx + dy - (dy >> 1);
}

//
// P_PointOnLineSide
//
int P_PointOnLineSide(fixed_t x, fixed_t y, line_t* line)
{
    return
        !line->dx ? (x <= line->v1->x ? (line->dy > 0) : (line->dy < 0)) :
        !line->dy ? (y <= line->v1->y ? (line->dx < 0) : (line->dx > 0)) :
        FixedMul(y - line->v1->y, line->dx >> FRACBITS) >=
        FixedMul(line->dy >> FRACBITS, x - line->v1->x);
}

//
// P_BoxOnLineSide
//
int P_BoxOnLineSide(fixed_t* tmbox, line_t* ld)
{
    int p1, p2;
    switch (ld->slopetype) {
    case ST_HORIZONTAL:
        p1 = tmbox[BOXTOP] > ld->v1->y;
        p2 = tmbox[BOXBOTTOM] > ld->v1->y;
        if (ld->dx < 0) { p1 ^= 1; p2 ^= 1; }
        break;
    case ST_VERTICAL:
        p1 = tmbox[BOXRIGHT] < ld->v1->x;
        p2 = tmbox[BOXLEFT] < ld->v1->x;
        if (ld->dy < 0) { p1 ^= 1; p2 ^= 1; }
        break;
    case ST_POSITIVE:
        p1 = P_PointOnLineSide(tmbox[BOXLEFT], tmbox[BOXTOP], ld);
        p2 = P_PointOnLineSide(tmbox[BOXRIGHT], tmbox[BOXBOTTOM], ld);
        break;
    case ST_NEGATIVE:
        p1 = P_PointOnLineSide(tmbox[BOXRIGHT], tmbox[BOXTOP], ld);
        p2 = P_PointOnLineSide(tmbox[BOXLEFT], tmbox[BOXBOTTOM], ld);
        break;
    default:
        return -1;
    }
    if (p1 == p2) 
        return p1;
    return -1;
}

//
// P_PointOnDivlineSide
//
int P_PointOnDivlineSide(fixed_t x, fixed_t y, divline_t* line)
{
    fixed_t dx, dy, left, right;

    if (!line->dx) 
        return (x <= line->x) ? (line->dy > 0) : (line->dy < 0);
    if (!line->dy) 
        return (y <= line->y) ? (line->dx < 0) : (line->dx > 0);

    dx = x - line->x;
    dy = y - line->y;

    // quick sign-bit path
    if ((line->dy ^ line->dx ^ dx ^ dy) & 0x80000000) {
        return ((line->dy ^ dx) & 0x80000000) ? 1 : 0;
    }

    left = FixedMul(line->dy >> 8, dx >> 8);
    right = FixedMul(dy >> 8, line->dx >> 8);
    return (right < left) ? 0 : 1;
}

void P_MakeDivline(line_t* li, divline_t* dl)
{
    dl->x = li->v1->x;
    dl->y = li->v1->y;
    dl->dx = li->dx;
    dl->dy = li->dy;
}

//
// P_GetIntersectPoint
//
void P_GetIntersectPoint(fixed_t* s1, fixed_t* s2, fixed_t* x, fixed_t* y)
{
    float a1 = F2D3D(s1[3]) - F2D3D(s1[1]);
    float b1 = F2D3D(s1[0]) - F2D3D(s1[2]);
    float c1 = F2D3D(s1[2]) * F2D3D(s1[1]) - F2D3D(s1[0]) * F2D3D(s1[3]);

    float a2 = F2D3D(s2[3]) - F2D3D(s2[1]);
    float b2 = F2D3D(s2[0]) - F2D3D(s2[2]);
    float c2 = F2D3D(s2[2]) * F2D3D(s2[1]) - F2D3D(s2[0]) * F2D3D(s2[3]);

    float d = a1 * b2 - a2 * b1;
    if (d == 0.0f) 
        return;

    *x = INT2F((fixed_t)((b1 * c2 - b2 * c1) / d));
    *y = INT2F((fixed_t)((a2 * c1 - a1 * c2) / d));
}

//
// P_InterceptVector
//
fixed_t P_InterceptVector(divline_t* v2, divline_t* v1)
{
    fixed_t frac, num, den;
    den = FixedMul(v1->dy >> 8, v2->dx) - FixedMul(v1->dx >> 8, v2->dy);
    if (den == 0) 
        return 0;

    num = FixedMul((v1->x - v2->x) >> 8, v1->dy)
        + FixedMul((v2->y - v1->y) >> 8, v1->dx);

    frac = FixedDiv(num, den);
    return frac;
}

//
// P_LineOpening
//
fixed_t   opentop;
fixed_t   openbottom;
fixed_t   openrange;
fixed_t   lowfloor;

sector_t* openfrontsector;
sector_t* openbacksector;

void P_LineOpening(line_t* linedef)
{
    if (linedef->sidenum[1] == NO_SIDE_INDEX) { 
        openrange = 0; 
        return; 
    }

    openfrontsector = linedef->frontsector;
    openbacksector = linedef->backsector;

    opentop = (openfrontsector->ceilingheight < openbacksector->ceilingheight)
        ? openfrontsector->ceilingheight
        : openbacksector->ceilingheight;

    if (openfrontsector->floorheight > openbacksector->floorheight) {
        openbottom = openfrontsector->floorheight;
        lowfloor = openbacksector->floorheight;
    }
    else {
        openbottom = openbacksector->floorheight;
        lowfloor = openfrontsector->floorheight;
    }
    openrange = opentop - openbottom;
}

//
// Thing Positional Settings
//
void P_UnsetThingPosition(mobj_t* thing)
{
    int blockx, blocky;

    if (!(thing->flags & MF_NOSECTOR)) {
        if (thing->snext) 
            thing->snext->sprev = thing->sprev;
        if (thing->sprev) 
            thing->sprev->snext = thing->snext;
        else              thing->subsector->sector->thinglist = thing->snext;
    }

    if (!(thing->flags & MF_NOBLOCKMAP)) {
        if (thing->bnext) 
            thing->bnext->bprev = thing->bprev;
        if (thing->bprev) 
            thing->bprev->bnext = thing->bnext;
        else {
            blockx = (thing->x - bmaporgx) >> MAPBLOCKSHIFT;
            blocky = (thing->y - bmaporgy) >> MAPBLOCKSHIFT;

            if (blockx >= 0 && blockx < bmapwidth &&
                blocky >= 0 && blocky < bmapheight) {
                blocklinks[blocky * bmapwidth + blockx] = thing->bnext;
            }
        }
    }
}

void P_SetThingPosition(mobj_t* thing)
{
    subsector_t* ss;
    sector_t* sec;
    int blockx, blocky;
    mobj_t** link;

    ss = R_PointInSubsector(thing->x, thing->y);
    thing->subsector = ss;

    if (!(thing->flags & MF_NOSECTOR)) {
        sec = ss->sector;
        thing->sprev = NULL;
        thing->snext = sec->thinglist;
        if (sec->thinglist) sec->thinglist->sprev = thing;
        sec->thinglist = thing;
    }

    if (!(thing->flags & MF_NOBLOCKMAP)) {
        blockx = (thing->x - bmaporgx) >> MAPBLOCKSHIFT;
        blocky = (thing->y - bmaporgy) >> MAPBLOCKSHIFT;

        if (blockx >= 0 && blockx < bmapwidth &&
            blocky >= 0 && blocky < bmapheight) {
            link = &blocklinks[blocky * bmapwidth + blockx];
            thing->bprev = NULL;
            thing->bnext = *link;
            if (*link) (*link)->bprev = thing;
            *link = thing;
        }
        else {
            thing->bnext = thing->bprev = NULL;
        }
    }
}

//
// Blockmap stuff
//
boolean P_BlockLinesIterator(int x, int y, boolean(*func)(line_t*))
{
    int offset;
    short* list;
    line_t* ld;

    if (x < 0 || y < 0 || x >= bmapwidth || y >= bmapheight) 
        return true;

    offset = y * bmapwidth + x;
    offset = (uint16_t)SHORT(*(blockmap + offset));

    for (list = (short*)blockmaplump + offset; *list != -1; list++) {
        ld = &lines[SHORT(*list)];

        if ((ld - lines) >= numlines) {
            I_Error("P_BlockLinesIterator: Linedef out of range");
        }

        if (ld->validcount == validcount) 
            continue;
        ld->validcount = validcount;

        if (!func(ld)) 
            return false;
    }
    return true;
}

boolean P_BlockThingsIterator(int x, int y, boolean(*func)(mobj_t*))
{
    mobj_t* mobj;

    if (x < 0 || y < 0 || x >= bmapwidth || y >= bmapheight) 
        return true;

    for (mobj = blocklinks[y * bmapwidth + x]; mobj; mobj = mobj->bnext) {
        if (!func(mobj)) 
            return false;
    }
    return true;
}

//
// Interp
//

boolean PIT_AddLineIntercepts(line_t* ld)
{
    int s1, s2;
    fixed_t frac;
    divline_t dl;

    if (trace.dx > FRACUNIT * 16 || trace.dy > FRACUNIT * 16 ||
        trace.dx < -FRACUNIT * 16 || trace.dy < -FRACUNIT * 16) {
        s1 = P_PointOnDivlineSide(ld->v1->x, ld->v1->y, &trace);
        s2 = P_PointOnDivlineSide(ld->v2->x, ld->v2->y, &trace);
    }
    else {
        s1 = P_PointOnLineSide(trace.x, trace.y, ld);
        s2 = P_PointOnLineSide(trace.x + trace.dx, trace.y + trace.dy, ld);
    }

    if (s1 == s2) 
        return true;

    P_MakeDivline(ld, &dl);
    frac = P_InterceptVector(&trace, &dl);

    if (frac < 0) 
        return true;
    if ((intercept_p - intercepts) >= MAXINTERCEPTS) 
        return true;

    intercept_p->frac = frac;
    intercept_p->isaline = true;
    intercept_p->d.line = ld;
    intercept_p++;
    return true;
}

boolean PIT_AddThingIntercepts(mobj_t* thing)
{
    fixed_t x1, y1, x2, y2;
    int s1, s2;
    boolean tracepositive = (trace.dx ^ trace.dy) > 0;
    divline_t dl;
    fixed_t frac;

    if (tracepositive) {
        x1 = thing->x - thing->radius; y1 = thing->y + thing->radius;
        x2 = thing->x + thing->radius; y2 = thing->y - thing->radius;
    }
    else {
        x1 = thing->x - thing->radius; y1 = thing->y - thing->radius;
        x2 = thing->x + thing->radius; y2 = thing->y + thing->radius;
    }

    s1 = P_PointOnDivlineSide(x1, y1, &trace);
    s2 = P_PointOnDivlineSide(x2, y2, &trace);
    if (s1 == s2) 
        return true;

    dl.x = x1; dl.y = y1; dl.dx = x2 - x1; dl.dy = y2 - y1;
    frac = P_InterceptVector(&trace, &dl);
    if (frac < 0) 
        return true;
    if ((intercept_p - intercepts) >= MAXINTERCEPTS) 
        return true;

    intercept_p->frac = frac;
    intercept_p->isaline = false;
    intercept_p->d.thing = thing;
    intercept_p++;
    return true;
}

boolean P_TraverseIntercepts(traverser_t func, fixed_t maxfrac)
{
    int64_t count = intercept_p - intercepts;
    intercept_t* scan, * in = 0;
    fixed_t dist;

    while (count--) {
        dist = INT_MAX;
        for (scan = intercepts; scan < intercept_p; scan++) {
            if (scan->frac < dist) { 
                dist = scan->frac; in = scan;
            }
        }

        if (dist > maxfrac) 
            return true;

        if (!func(in)) 
            return false;
        in->frac = INT_MAX;
    }
    return true;
}

void T_TraceDrawer(tracedrawer_t* tdrawer)
{
    if (gametic > tdrawer->tic) {
        P_RemoveThinker(&tdrawer->thinker);
    }
}

//
// P_PathTraverse
//
boolean P_PathTraverse(fixed_t x1, fixed_t y1, fixed_t x2, fixed_t y2,
    int flags, boolean(*trav)(intercept_t*))
{
    fixed_t xt1, yt1, xt2, yt2, xstep, ystep, partial, xintercept, yintercept;
    int mapx, mapy, mapxstep, mapystep, count;

    validcount++;
    intercept_p = intercepts;

    if (((x1 - bmaporgx) & (MAPBLOCKSIZE - 1)) == 0) x1 += FRACUNIT;
    if (((y1 - bmaporgy) & (MAPBLOCKSIZE - 1)) == 0) y1 += FRACUNIT;

    trace.x = x1; 
    trace.y = y1; 
    trace.dx = x2 - x1; 
    trace.dy = y2 - y1;

    x1 -= bmaporgx; y1 -= bmaporgy; xt1 = x1 >> MAPBLOCKSHIFT; yt1 = y1 >> MAPBLOCKSHIFT;
    x2 -= bmaporgx; y2 -= bmaporgy; xt2 = x2 >> MAPBLOCKSHIFT; yt2 = y2 >> MAPBLOCKSHIFT;

    if (xt2 > xt1) { 
        mapxstep = 1; 
        partial = FRACUNIT - ((x1 >> MAPBTOFRAC) & (FRACUNIT - 1)); 
        ystep = FixedDiv(y2 - y1, D_abs(x2 - x1)); 
    }
    else if (xt2 < xt1) { 
        mapxstep = -1; partial = (x1 >> MAPBTOFRAC) & (FRACUNIT - 1);         
        ystep = FixedDiv(y2 - y1, D_abs(x2 - x1)); 
    }
    else { 
        mapxstep = 0; partial = FRACUNIT; ystep = 256 * FRACUNIT; 
    }

    yintercept = (y1 >> MAPBTOFRAC) + FixedMul(partial, ystep);

    if (yt2 > yt1) { mapystep = 1; 
    partial = FRACUNIT - ((y1 >> MAPBTOFRAC) & (FRACUNIT - 1)); 
    xstep = FixedDiv(x2 - x1, D_abs(y2 - y1)); 
    }
    else if (yt2 < yt1) { mapystep = -1; 
    partial = (y1 >> MAPBTOFRAC) & (FRACUNIT - 1);         
    xstep = FixedDiv(x2 - x1, D_abs(y2 - y1)); 
    }
    else { 
        mapystep = 0; 
        partial = FRACUNIT; 
        xstep = 256 * FRACUNIT; 
    }

    xintercept = (x1 >> MAPBTOFRAC) + FixedMul(partial, xstep);

    mapx = xt1; mapy = yt1;

    for (count = 0; count < 64; count++) {
        if ((flags & PT_ADDLINES) && !P_BlockLinesIterator(mapx, mapy, PIT_AddLineIntercepts))  
            return false;
        if ((flags & PT_ADDTHINGS) && !P_BlockThingsIterator(mapx, mapy, PIT_AddThingIntercepts)) 
            return false;

        if (mapx == xt2 && mapy == yt2) 
            break;

        if (F2INT(yintercept) == mapy) {
            yintercept += ystep; mapx += mapxstep;
        }
        else if (F2INT(xintercept) == mapx) {
            xintercept += xstep; mapy += mapystep;
        }
    }
    return P_TraverseIntercepts(trav, FRACUNIT);
}
