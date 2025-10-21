// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 1993-1997 Id Software, Inc.
// Copyright(C) 1997 Midway Home Entertainment, Inc
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
//      Movement, collision handling.
//      Shooting and aiming.
//
//-----------------------------------------------------------------------------

#include "m_fixed.h"
#include "m_random.h"
#include "m_misc.h"
#include "doomdef.h"
#include "p_local.h"
#include "s_sound.h"
#include "doomstat.h"
#include "sounds.h"
#include "tables.h"
#include "r_sky.h"
#include "r_main.h"
#include "con_console.h"

// atsb: Reverse Engineered

fixed_t         tmbbox[4];
mobj_t* tmthing;
int             tmflags;
fixed_t         tmx;
fixed_t         tmy;

mobj_t* blockthing;

int             floatok;

fixed_t         tmfloorz;
fixed_t         tmceilingz;
fixed_t         tmdropoffz;
line_t* tmhitline;

line_t* thingspec[MAXTHINGSPEC];

int             numthingspec = 0;

static mobj_t* usething = NULL;

static int     usecontext = false;
static int     displaycontext = false;

line_t* contextline = NULL;

mobj_t* linetarget;
mobj_t* shootthing;

fixed_t shootdirx, shootdiry, shootdirz;
fixed_t shootz;

int     la_damage;

fixed_t attackrange;

fixed_t aimslope;
fixed_t aimpitch;

extern fixed_t topslope;
extern fixed_t bottomslope;
extern fixed_t laserhit_x, laserhit_y, laserhit_z;

//
// P_CheckThingCollision
//
static int P_CheckThingCollision(mobj_t* thing)
{
    if (netgame && (tmthing->type == MT_PLAYER && thing->type == MT_PLAYER)) {
        return true;
    }

    const fixed_t blockdist = thing->radius + tmthing->radius;

    fixed_t dx = D_abs(thing->x - tmx);
    fixed_t dy = D_abs(thing->y - tmy);

    if (dx >= blockdist || dy >= blockdist) {
        return true;
    }

    fixed_t dist_squared = FixedMul(dx, dx) + FixedMul(dy, dy);
    fixed_t radius_squared = FixedMul(blockdist, blockdist);

    if (dist_squared >= radius_squared) {
        return true;
    }

    return false;
}

//
// P_BlockMapBox
//
extern byte forcecollision;

static void P_BlockMapBox(fixed_t* bbox, fixed_t x, fixed_t y, mobj_t* thing)
{
    // [D64] does not use MAXRADIUS
    tmbbox[BOXTOP] = y + thing->radius;
    tmbbox[BOXBOTTOM] = y - thing->radius;
    tmbbox[BOXRIGHT] = x + thing->radius;
    tmbbox[BOXLEFT] = x - thing->radius;

    bbox[BOXLEFT] = (tmbbox[BOXLEFT] - bmaporgx) >> MAPBLOCKSHIFT;
    bbox[BOXRIGHT] = (tmbbox[BOXRIGHT] - bmaporgx) >> MAPBLOCKSHIFT;
    bbox[BOXBOTTOM] = (tmbbox[BOXBOTTOM] - bmaporgy) >> MAPBLOCKSHIFT;
    bbox[BOXTOP] = (tmbbox[BOXTOP] - bmaporgy) >> MAPBLOCKSHIFT;

    if (bbox[BOXLEFT] < 0) bbox[BOXLEFT] = 0;
    if (bbox[BOXRIGHT] < 0) bbox[BOXRIGHT] = 0;
    if (bbox[BOXLEFT] >= bmapwidth)  bbox[BOXLEFT] = bmapwidth - 1;
    if (bbox[BOXRIGHT] >= bmapwidth)  bbox[BOXRIGHT] = bmapwidth - 1;
    if (bbox[BOXBOTTOM] < 0) bbox[BOXBOTTOM] = 0;
    if (bbox[BOXTOP] < 0) bbox[BOXTOP] = 0;
    if (bbox[BOXBOTTOM] >= bmapheight) bbox[BOXBOTTOM] = bmapheight - 1;
    if (bbox[BOXTOP] >= bmapheight) bbox[BOXTOP] = bmapheight - 1;
}

//
// MOVEMENT ITERATOR FUNCTIONS
//
boolean PIT_CheckLine(line_t* ld)
{
    sector_t* sector;

    if (tmbbox[BOXRIGHT] <= ld->bbox[BOXLEFT] ||
        tmbbox[BOXLEFT] >= ld->bbox[BOXRIGHT] ||
        tmbbox[BOXTOP] <= ld->bbox[BOXBOTTOM] ||
        tmbbox[BOXBOTTOM] >= ld->bbox[BOXTOP]) {
        return true;
    }

    if (P_BoxOnLineSide(tmbbox, ld) != -1) {
        return true;
    }

    // one-sided?
    if (!ld->backsector) {
        if (tmthing->flags & MF_MISSILE) tmhitline = ld;
        return false;
    }

    if (!(tmthing->flags & MF_MISSILE)) {
        if (ld->flags & ML_BLOCKING)
            return false;
        if (!tmthing->player && (ld->flags & ML_BLOCKMONSTERS))
            return false;
        if (!(tmthing->flags & MF_COUNTKILL) && (ld->flags & ML_BLOCKPLAYER))
            return false;
    }

    // [d64] don't cross mid-pegged lines
    // styd: fixes block projectile to not block things and only blocks projectiles
    if (!(tmthing->flags & MF_SOLID) && ld->flags & ML_BLOCKPROJECTILES) {
        tmhitline = ld;
        return false;
    }

    // [kex] midpoint-only check
    if ((tmthing->blockflag & BF_MIDPOINTONLY) && ld->backsector) {
        if (tmthing->subsector->sector != ld->backsector)
            return true;
    }

    sector = ld->frontsector;
    if (sector->ceilingheight == sector->floorheight) {
        tmhitline = ld;
        return false;
    }

    sector = ld->backsector;
    if (sector->ceilingheight == sector->floorheight) {
        tmhitline = ld;
        return false;
    }

    // opening
    P_LineOpening(ld);

    if (opentop < tmceilingz) { 
        tmceilingz = opentop; tmhitline = ld; 
    }
    if (openbottom > tmfloorz) tmfloorz = openbottom;
    if (lowfloor < tmdropoffz) tmdropoffz = lowfloor;

    // special crossing lines queued
    if (ld->special & MLU_CROSS) {
        if (numthingspec < MAXSPECIALCROSS) {
            thingspec[numthingspec++] = ld;
        }
        else {
            CON_Warnf("PIT_CheckLine: spechit overflow!\n");
        }
    }

    return true;
}

boolean PIT_CheckThing(mobj_t* thing)
{
    if (!(thing->flags & (MF_SOLID | MF_SPECIAL | MF_SHOOTABLE)))
        return true;
    if (P_CheckThingCollision(thing))
        return true;
    if (thing == tmthing)
        return true;

    if (tmthing->blockflag & BF_MOBJPASS) {
        if (tmthing->z >= thing->z + thing->height && !(thing->flags & MF_SPECIAL))
            return true;
        if (tmthing->z + tmthing->height < thing->z && !(thing->flags & MF_SPECIAL))
            return true;
    }

    if (tmthing->flags & MF_SKULLFLY) { 
        blockthing = thing; 
        return false; 
    }

    if (tmthing->flags & MF_MISSILE) {
        if (tmthing->z > thing->z + thing->height) 
            return true;
        if (tmthing->z + tmthing->height < thing->z) 
            return true;

        if (tmthing->target && tmthing->target->type == thing->type) {
            if (thing == tmthing->target) 
                return true;
            if (thing->type != MT_PLAYER) 
                return false;
        }

        if (!(thing->flags & MF_SHOOTABLE)) {
            return !(thing->flags & MF_SOLID);
        }

        blockthing = thing;
        return false;
    }

    const int solid = thing->flags & MF_SOLID;

    if (thing->flags & MF_SPECIAL) {
        if (tmflags & MF_PICKUP) {
            blockthing = thing;
            return true;
        }
    }
    return !solid;
}

//
// MOVEMENT CLIPPING
//
boolean P_CheckPosition(mobj_t* thing, fixed_t x, fixed_t y)
{
    int bx, by;
    subsector_t* newsubsec;
    fixed_t bbox[4];

    tmthing = thing;
    tmflags = thing->flags;
    tmx = x; tmy = y;

    newsubsec = R_PointInSubsector(x, y);
    tmhitline = NULL;
    blockthing = NULL;

    tmfloorz = tmdropoffz = newsubsec->sector->floorheight;
    tmceilingz = newsubsec->sector->ceilingheight;

    validcount++;
    numthingspec = 0;

    if (tmflags & MF_NOCLIP) 
        return true;

    P_BlockMapBox(bbox, x, y, tmthing);

    for (bx = bbox[BOXLEFT]; bx <= bbox[BOXRIGHT]; bx++)
        for (by = bbox[BOXBOTTOM]; by <= bbox[BOXTOP]; by++)
            if (!P_BlockThingsIterator(bx, by, PIT_CheckThing)) 
                return false;

    for (bx = bbox[BOXLEFT]; bx <= bbox[BOXRIGHT]; bx++)
        for (by = bbox[BOXBOTTOM]; by <= bbox[BOXTOP]; by++)
            if (!P_BlockLinesIterator(bx, by, PIT_CheckLine)) 
                return false;

    return true;
}

//
// P_TryMove
//
boolean P_TryMove(mobj_t* thing, fixed_t x, fixed_t y)
{
    fixed_t oldx, oldy;
    int side, oldside;
    line_t* ld;

    floatok = false;
    if (!P_CheckPosition(thing, x, y)) 
        return false;

    if (!(thing->flags & MF_NOCLIP)) {
        if (tmceilingz - tmfloorz < thing->height) 
            return false;
        floatok = true;
        if (!(thing->flags & MF_TELEPORT) && tmceilingz - thing->z < thing->height) 
            return false;
        if (!(thing->flags & MF_TELEPORT) && tmfloorz - thing->z > 24 * FRACUNIT) 
            return false;
        if (!(thing->flags & (MF_DROPOFF | MF_FLOAT)) && tmfloorz - tmdropoffz > 24 * FRACUNIT) 
            return false;
    }

    P_UnsetThingPosition(thing);

    oldx = thing->x; oldy = thing->y;
    thing->floorz = tmfloorz;
    thing->ceilingz = tmceilingz;
    thing->x = x; thing->y = y;

    P_SetThingPosition(thing);

    if (!(thing->flags & (MF_NOCLIP | MF_TELEPORT))) {
        while (numthingspec > 0) {
            ld = thingspec[--numthingspec];
            side = P_PointOnLineSide(thing->x, thing->y, ld);
            oldside = P_PointOnLineSide(oldx, oldy, ld);
            if (side != oldside) {
                if (!(ld->flags & ML_TRIGGERFRONT) || (side)) {
                    P_UseSpecialLine(thing, ld, oldside);
                }
            }
        }
    }

    floatok = true;
    return true;
}

//
// P_PlayerMove — like P_TryMove but defers pickups to 1/tic
//
int P_PlayerMove(mobj_t* thing, fixed_t x, fixed_t y)
{
    int moveok = P_TryMove(thing, x, y);

    if (thing->flags & MF_PICKUP && blockthing) {
        mobj_t* touchThing = blockthing;
        if (touchThing->flags & MF_SPECIAL) {
            P_TouchSpecialThing(touchThing, thing);
            blockthing = NULL;
        }
    }
    return moveok;
}

//
// TELEPORT MOVE
//
boolean P_TeleportMove(mobj_t* thing, fixed_t x, fixed_t y)
{
    int bx, by;
    subsector_t* newsubsec;
    fixed_t bbox[4];

    tmthing = thing;
    tmflags = thing->flags;
    tmx = x; tmy = y;

    newsubsec = R_PointInSubsector(x, y);
    tmhitline = NULL;

    tmfloorz = tmdropoffz = newsubsec->sector->floorheight;
    tmceilingz = newsubsec->sector->ceilingheight;

    validcount++;
    numthingspec = 0;

    P_BlockMapBox(bbox, x, y, tmthing);

    for (bx = bbox[BOXLEFT]; bx <= bbox[BOXRIGHT]; bx++)
        for (by = bbox[BOXBOTTOM]; by <= bbox[BOXTOP]; by++)
            if (!P_BlockThingsIterator(bx, by, PIT_CheckThing)) 
                return false;

    P_UnsetThingPosition(thing);

    thing->floorz = tmfloorz;
    thing->ceilingz = tmceilingz;
    thing->x = x; thing->y = y;

    P_SetThingPosition(thing);
    return true;
}

//
// P_ThingHeightClip
//
static int P_ThingHeightClip(mobj_t* thing)
{
    const int onfloor = (thing->z == thing->floorz);

    P_CheckPosition(thing, thing->x, thing->y);

    thing->floorz = tmfloorz;
    thing->ceilingz = tmceilingz;

    if (onfloor) {
        thing->z = thing->floorz;
    }
    else if (thing->z + thing->height > thing->ceilingz) {
        thing->z = thing->ceilingz - thing->height;
    }

    if (thing->ceilingz - thing->floorz < thing->height) 
        return false;
    return true;
}

//
// PIT_CheckMobjZ
//
mobj_t* onmobj;

static boolean PIT_CheckMobjZ(mobj_t* thing)
{
    if (!(thing->flags & (MF_SOLID | MF_SPECIAL | MF_SHOOTABLE))) 
        return true;
    if (P_CheckThingCollision(thing)) 
        return true;
    if (thing == tmthing) 
        return true;

    if (tmthing->z > thing->z + thing->height) 
        return true;
    if (tmthing->z + tmthing->height < thing->z) 
        return true;

    if (thing->flags & MF_SOLID) onmobj = thing;
    return !(thing->flags & MF_SOLID);
}

//
// P_CheckOnMobj
//
extern void P_ZMovement(mobj_t* mo);

mobj_t* P_CheckOnMobj(mobj_t* thing)
{
    int bx, by;
    subsector_t* newsubsec;
    fixed_t x = thing->x, y = thing->y;
    mobj_t oldmo;
    fixed_t bbox[4];

    tmthing = thing;
    tmflags = thing->flags;
    oldmo = *thing;
    P_ZMovement(tmthing);

    tmx = x; tmy = y;
    newsubsec = R_PointInSubsector(x, y);
    tmhitline = NULL;

    tmfloorz = tmdropoffz = newsubsec->sector->floorheight;
    tmceilingz = newsubsec->sector->ceilingheight;

    validcount++;
    numthingspec = 0;

    if (tmflags & MF_NOCLIP) 
        return NULL;

    P_BlockMapBox(bbox, x, y, tmthing);

    for (bx = bbox[BOXLEFT]; bx <= bbox[BOXRIGHT]; bx++) {
        for (by = bbox[BOXBOTTOM]; by <= bbox[BOXTOP]; by++) {
            if (!P_BlockThingsIterator(bx, by, PIT_CheckMobjZ)) {
                *tmthing = oldmo;
                return onmobj;
            }
        }
    }

    *tmthing = oldmo;
    return NULL;
}

//
// SLIDE MOVE
//
fixed_t   bestslidefrac;
line_t* bestslideline;

mobj_t* slidemo;

fixed_t   tmxmove;
fixed_t   tmymove;

boolean PTR_SlideTraverse(intercept_t* in)
{
    line_t* li = in->d.line;

    if (!(li->flags & ML_TWOSIDED)) {
        if (P_PointOnLineSide(slidemo->x, slidemo->y, li)) 
            return true;
        goto isblocking;
    }

    P_LineOpening(li);

    if (openrange < slidemo->height)         
        goto isblocking;
    if (opentop - slidemo->z < slidemo->height) 
        goto isblocking;
    if (openbottom - slidemo->z > 24 * FRACUNIT) 
        goto isblocking;

    return true;

isblocking:
    if (in->frac < bestslidefrac) {
        bestslidefrac = in->frac;
        bestslideline = li;
    }
    return false;
}

void P_SlideMove(mobj_t* mo)
{
    fixed_t leadx, leady, trailx, traily, newx, newy;
    int hitcount = 0;
    line_t* ld;
    int an1, an2;

    slidemo = mo;

retry:
    if (++hitcount == 3) goto stairstep;

    if (mo->momx > 0) { leadx = mo->x + mo->radius; trailx = mo->x - mo->radius; }
    else { leadx = mo->x - mo->radius; trailx = mo->x + mo->radius; }

    if (mo->momy > 0) { leady = mo->y + mo->radius; traily = mo->y - mo->radius; }
    else { leady = mo->y - mo->radius; traily = mo->y + mo->radius; }

    bestslidefrac = FRACUNIT + 1;

    P_PathTraverse(leadx, leady, leadx + mo->momx, leady + mo->momy, PT_ADDLINES, PTR_SlideTraverse);
    P_PathTraverse(trailx, leady, trailx + mo->momx, leady + mo->momy, PT_ADDLINES, PTR_SlideTraverse);
    P_PathTraverse(leadx, traily, leadx + mo->momx, traily + mo->momy, PT_ADDLINES, PTR_SlideTraverse);

    if (bestslidefrac == FRACUNIT + 1) {
    stairstep:
        if (!P_PlayerMove(mo, mo->x, mo->y + mo->momy)) {
            if (!P_PlayerMove(mo, mo->x + mo->momx, mo->y)) {
                mo->momx = 0;
                mo->momy = 0;
            }
        }
        return;
    }

    bestslidefrac -= 0x800;
    if (bestslidefrac > 0) {
        newx = FixedMul(mo->momx, bestslidefrac);
        newy = FixedMul(mo->momy, bestslidefrac);
        if (!P_PlayerMove(mo, mo->x + newx, mo->y + newy)) {
            bestslidefrac = FRACUNIT;
            goto hitslideline;
        }
    }

    bestslidefrac = FRACUNIT - (bestslidefrac + 0x800);
    if (bestslidefrac > FRACUNIT) bestslidefrac = FRACUNIT;
    if (bestslidefrac <= 0) return;

hitslideline:
    ld = bestslideline;

    if (ld->slopetype == ST_HORIZONTAL) tmymove = 0;
    else                                tmymove = FixedMul(mo->momy, bestslidefrac);

    if (ld->slopetype == ST_VERTICAL)   tmxmove = 0;
    else                                tmxmove = FixedMul(mo->momx, bestslidefrac);

    an1 = finecosine[ld->angle];
    an2 = finesine[ld->angle];

    if (P_PointOnLineSide(mo->x, mo->y, bestslideline)) {
        an1 = -an1;
        an2 = -an2;
    }

    newx = FixedMul(tmxmove, an1);
    newy = FixedMul(tmymove, an2);

    mo->momx = FixedMul(newx + newy, an1);
    mo->momy = FixedMul(newx + newy, an2);

    if (!P_PlayerMove(mo, mo->x + mo->momx, mo->y + mo->momy)) 
        goto retry;
}

//
// LINE ATTACK / AIM
//

boolean PTR_AimTraverse(intercept_t* in)
{
    line_t* li;
    mobj_t* th;
    fixed_t linebottomslope, linetopslope, thingtopslope, thingbottomslope, dist;

    if (in->isaline) {
        li = in->d.line;

        if (!(li->flags & ML_TWOSIDED)) 
            return false;

        P_LineOpening(li);
        if (openbottom >= opentop) 
            return false;

        dist = FixedMul(attackrange, in->frac);

        if (li->frontsector->floorheight != li->backsector->floorheight) {
            linebottomslope = FixedDiv(openbottom - shootz, dist);
            if (linebottomslope > bottomslope) bottomslope = linebottomslope;
        }
        if (li->frontsector->ceilingheight != li->backsector->ceilingheight) {
            linetopslope = FixedDiv(opentop - shootz, dist);
            if (linetopslope < topslope) topslope = linetopslope;
        }

        if (topslope <= bottomslope) 
            return false;
        return true;
    }

    th = in->d.thing;
    if (th == shootthing) 
        return true;
    if (!(th->flags & MF_SHOOTABLE)) 
        return true;

    dist = FixedMul(attackrange, in->frac);
    thingtopslope = FixedDiv(th->z + th->height - shootz, dist);
    if (thingtopslope < bottomslope) 
        return true;

    thingbottomslope = FixedDiv(th->z - shootz, dist);
    if (thingbottomslope > topslope) 
        return true;

    if (thingtopslope > topslope)      thingtopslope = topslope;
    if (thingbottomslope < bottomslope) thingbottomslope = bottomslope;

    aimslope = (thingtopslope + thingbottomslope) >> 1;
    linetarget = th;
    return false;
}

boolean PTR_ShootTraverse(intercept_t* in)
{
    fixed_t x = 0, y = 0, z = 0, frac, slope, dist, thingtopslope, thingbottomslope;
    line_t* li;
    mobj_t* th;
    int hitplane = false, lineside;
    sector_t* sidesector;
    fixed_t hitz;

    if (in->isaline) {
        li = in->d.line;

        lineside = P_PointOnLineSide(shootthing->x, shootthing->y, li);

        sidesector = lineside ? li->backsector : li->frontsector;
        dist = FixedMul(attackrange, in->frac);
        hitz = shootz + FixedMul(dcos(shootthing->pitch - ANG90), dist);

        if (li->flags & ML_TWOSIDED && li->backsector) {
            P_LineOpening(li);
            if ((li->frontsector->floorheight == li->backsector->floorheight
                || (slope = FixedDiv(openbottom - shootz, dist)) <= aimslope)
                && (li->frontsector->ceilingheight == li->backsector->ceilingheight
                    || (slope = FixedDiv(opentop - shootz, dist)) >= aimslope)) {
                return true;
            }
        }

        if (sidesector) {
            if (!(hitz > sidesector->floorheight && hitz < sidesector->ceilingheight)) {
                hitplane = true;
            }
        }

        if (hitplane) {
            plane_t* plane;
            fixed_t den;

            if (hitz <= sidesector->floorheight) plane = &sidesector->floorplane;
            else                                  plane = &sidesector->ceilingplane;

            den = FixedDot(plane->a, plane->b, plane->c, shootdirx, shootdiry, shootdirz);

            if (den != 0) {
                fixed_t hitdist;
                fixed_t num = FixedDot(plane->a, plane->b, plane->c, trace.x, trace.y, shootz) + plane->d;
                hitdist = FixedDiv(-num, den);

                frac = FixedDiv(hitdist, attackrange);
                x = trace.x + FixedMul(FixedMul(aimpitch, trace.dx), frac);
                y = trace.y + FixedMul(FixedMul(aimpitch, trace.dy), frac);
                z = hitz;
            }
        }
        else {
            frac = in->frac - FixedDiv(4 * FRACUNIT, attackrange);
            x = trace.x + FixedMul(trace.dx, frac);
            y = trace.y + FixedMul(trace.dy, frac);
            z = shootz + FixedMul(aimslope, FixedMul(frac, attackrange));
        }

        // Styd: Fixes a vanilla bug or the switches that are in the floor it activates even if it is in the floor when the player shoots them
        if (!hitplane && li->special & MLU_SHOOT) {
            P_UseSpecialLine(shootthing, li, lineside);
        }

        if (sides[li->sidenum[0]].midtexture == 1
            || sides[li->sidenum[0]].toptexture == 1
            || sides[li->sidenum[0]].bottomtexture == 1) {
            return false;
        }

        if (li->frontsector->ceilingpic == skyflatnum) {
            if (z > li->frontsector->ceilingheight) 
                return false;
            if (li->backsector && li->backsector->ceilingpic == skyflatnum &&
                li->backsector->ceilingheight < z) 
                return false;
        }

        if (attackrange == LASERRANGE) {
            laserhit_x = x; laserhit_y = y; laserhit_z = z;
        }
        else {
            P_SpawnPuff(x, y, z);
        }
        return false;
    }

    th = in->d.thing;
    if (th == shootthing) 
        return true;
    if (!(th->flags & MF_SHOOTABLE)) 
        return true;

    dist = FixedMul(attackrange, in->frac);
    thingtopslope = FixedDiv(th->z + th->height - shootz, dist);
    if (thingtopslope < aimslope) 
        return true;

    thingbottomslope = FixedDiv(th->z - shootz, dist);
    if (thingbottomslope > aimslope) 
        return true;

    frac = in->frac - FixedDiv(10 * FRACUNIT, attackrange);
    x = trace.x + FixedMul(trace.dx, frac);
    y = trace.y + FixedMul(trace.dy, frac);
    z = shootz + FixedMul(aimslope, FixedMul(frac, attackrange));

    if (attackrange == LASERRANGE) {
        laserhit_x = x; laserhit_y = y; laserhit_z = z;
    }
    else if (in->d.thing->flags & MF_NOBLOOD) {
        P_SpawnPuff(x, y, z);
    }
    else if (th->type == MT_BRUISER2) {
        P_SpawnBloodGreen(x, y, z, la_damage);
    }
    else if (th->type == MT_IMP2) {
        P_SpawnBloodPurple(x, y, z, la_damage);
    }
    else if (th->type == MT_SKULL) {
        P_SpawnPuff(x, y, z);
    }
    else {
        P_SpawnBlood(x, y, z, la_damage);
    }

    if (la_damage) {
        P_DamageMobj(th, shootthing, shootthing, la_damage);
    }
    return false;
}

fixed_t P_AimLineAttack(mobj_t* t1, angle_t angle, fixed_t zheight, fixed_t distance)
{
    int flags;
    fixed_t x2, y2, pitch = 0, dist;

    angle >>= ANGLETOFINESHIFT;
    dist = distance >> FRACBITS;

    shootthing = t1;

    x2 = t1->x + dist * finecosine[angle];
    y2 = t1->y + dist * finesine[angle];

    topslope = 120 * FRACUNIT / 160;
    bottomslope = -120 * FRACUNIT / 160;

    attackrange = distance;
    linetarget = NULL;
    flags = PT_ADDLINES | PT_ADDTHINGS | PT_EARLYOUT;

    if (!zheight) shootz = t1->z + (t1->height >> 1) + 12 * FRACUNIT;
    else          shootz = t1->z + zheight;

    if (t1->player) {
        pitch = dcos(D_abs(t1->pitch - ANG90));
        if (distance != (ATTACKRANGE + 1)) {
            if (!t1->player->autoaim && distance != MELEERANGE && distance != (MELEERANGE + 1)) {
                flags &= ~PT_ADDTHINGS;
            }
        }
    }

    P_PathTraverse(t1->x, t1->y, x2, y2, flags, PTR_AimTraverse);

    if (linetarget) return aimslope;

    if (t1->player) {
        aimslope = pitch;
        if (D_abs(aimslope) > 0x40) return aimslope;
    }
    return 0;
}

void P_LineAttack(mobj_t* t1, angle_t angle, fixed_t distance, fixed_t slope, int damage)
{
    fixed_t x2, y2, dist;

    angle >>= ANGLETOFINESHIFT;
    dist = distance >> FRACBITS;

    shootthing = t1;
    la_damage = damage;

    x2 = t1->x + dist * finecosine[angle];
    y2 = t1->y + dist * finesine[angle];

    shootz = t1->z + (t1->height >> 1) + 12 * FRACUNIT;

    attackrange = distance;
    aimslope = slope;
    aimpitch = dcos(shootthing->pitch);

    shootdirx = FixedMul(aimpitch, dcos(shootthing->angle));
    shootdiry = FixedMul(aimpitch, dsin(shootthing->angle));
    shootdirz = dsin(shootthing->pitch);
    laserhit_x = t1->x;
    laserhit_y = t1->y;
    laserhit_z = t1->z;

    P_PathTraverse(t1->x, t1->y, x2, y2, PT_ADDLINES | PT_ADDTHINGS | PT_EARLYOUT, PTR_ShootTraverse);
}

//
// USE LINES
//

static int P_CheckUseHeight(line_t* line, mobj_t* thing)
{
    int     flags = line->flags & (ML_CHECKFLOORHEIGHT | ML_SWITCHX08);
    fixed_t rowoffset = sides[line->sidenum[0]].rowoffset;
    fixed_t check;

    switch (flags) {
    case ML_SWITCHX08:
        if (!line->backsector) return true;
        check = (line->backsector->ceilingheight + rowoffset) + (32 * FRACUNIT);
        break;
    case ML_CHECKFLOORHEIGHT:
        if (!line->backsector) return true;
        check = (line->backsector->floorheight + rowoffset) - (32 * FRACUNIT);
        break;
    case (ML_CHECKFLOORHEIGHT | ML_SWITCHX08):
        check = (line->frontsector->floorheight + rowoffset) + (32 * FRACUNIT);
        break;
    default:
        return true;
    }

    if (check < thing->z) return false;
    if ((thing->z + (64 * FRACUNIT)) < check) 
        return false;
    return true;
}

boolean PTR_UseTraverse(intercept_t* in)
{
    if (!in->d.line->special) {
        P_LineOpening(in->d.line);
        if (openrange <= 0) {
            if (!usecontext) S_StartSound(usething, sfx_noway);
            return false;
        }
        return true;
    }

    if (P_PointOnLineSide(usething->x, usething->y, in->d.line) == 1) 
        return true;

    if (!(in->d.line->special & MLU_USE) || !P_CheckUseHeight(in->d.line, usething)) {
        if (!usecontext) S_StartSound(usething, sfx_noway);
        return false;
    }

    if (!usecontext) P_UseSpecialLine(usething, in->d.line, 0);

    displaycontext = true;
    contextline = in->d.line;
    return false;
}

boolean P_UseLines(player_t* player, int showcontext)
{
    int angle;
    fixed_t x1, y1, x2, y2;

    usething = player->mo;
    usecontext = showcontext;
    displaycontext = false;
    contextline = NULL;

    angle = player->mo->angle >> ANGLETOFINESHIFT;
    x1 = player->mo->x;
    y1 = player->mo->y;
    x2 = x1 + (USERANGE >> FRACBITS) * finecosine[angle];
    y2 = y1 + (USERANGE >> FRACBITS) * finesine[angle];

    P_PathTraverse(x1, y1, x2, y2, PT_ADDLINES, PTR_UseTraverse);
    return displaycontext;
}

//
// RADIUS ATTACK
//
mobj_t* bombsource;
mobj_t* bombspot;
int     bombdamage;

boolean PIT_RadiusAttack(mobj_t* thing)
{
    fixed_t dx, dy;
    int dist;

    if (!(thing->flags & MF_SHOOTABLE)) 
        return true;

    if (thing->type == MT_CYBORG || thing->type == MT_SPIDER) 
        return true;

    if ((thing->type == MT_SKULL || thing->type == MT_PAIN) &&
        bombsource && bombsource->type == MT_SKULL) 
        return true;

    dx = D_abs(thing->x - bombspot->x);
    dy = D_abs(thing->y - bombspot->y);

    {
        fixed_t maxf = (dx > dy) ? dx : dy;
        fixed_t adj = maxf - thing->radius;
        dist = (adj >= 0) ? (adj >> FRACBITS) : 0;
    }

    if (dist >= bombdamage) 
        return true;

    if (P_CheckSight(thing, bombspot) != 0)
        P_DamageMobj(thing, bombspot, bombsource, bombdamage - dist);

    return true;
}

void P_RadiusAttack(mobj_t* spot, mobj_t* source, int damage)
{
    int x, y, xl, xh, yl, yh;
    fixed_t dist;

    dist = (damage + MAXRADIUS) << FRACBITS;
    yh = (spot->y + dist - bmaporgy) >> MAPBLOCKSHIFT;
    yl = (spot->y - dist - bmaporgy) >> MAPBLOCKSHIFT;
    xh = (spot->x + dist - bmaporgx) >> MAPBLOCKSHIFT;
    xl = (spot->x - dist - bmaporgx) >> MAPBLOCKSHIFT;
    bombspot = spot;
    bombsource = source;
    bombdamage = damage;

    for (y = yl; y <= yh; y++)
        for (x = xl; x <= xh; x++)
            P_BlockThingsIterator(x, y, PIT_RadiusAttack);
}

//
// SECTOR HEIGHT CHANGING
//
static int crushchange;
static int nofit;

boolean PIT_ChangeSector(mobj_t* thing)
{
    mobj_t* mo = NULL;

    if (P_ThingHeightClip(thing)) 
        return true;

    if (thing->health <= 0 && thing->tics == -1) {
        if (thing->state != &states[S_CORPSE]) {
            P_SetMobjState(thing, S_CORPSE);
            S_StartSound(thing, sfx_slop);
        }
        return true;
    }

    if (thing->flags & MF_DROPPED) {
        P_RemoveMobj(thing);
        return true;
    }

    if (!(thing->flags & MF_SHOOTABLE)) 
        return true;

    nofit = true;

    if (crushchange == 2) {
        P_DamageMobj(thing, NULL, NULL, 99999);
        return true;
    }
    else {
        if (crushchange && !(leveltime & 3)) {
            P_DamageMobj(thing, NULL, NULL, 10);

            if (thing->type == MT_BRUISER2) {
                mo = P_SpawnMobj(thing->x, thing->y, thing->z + thing->height / 2, MT_BLOOD_GREEN);
            }
            else if (thing->type == MT_IMP2) {
                mo = P_SpawnMobj(thing->x, thing->y, thing->z + thing->height / 2, MT_BLOOD_PURPLE);
            }
            else if (thing->type == MT_SKULL) {
                mo = P_SpawnMobj(thing->x, thing->y, thing->z + thing->height / 2, MT_SMOKE_GRAY);
            }
            else {
                mo = P_SpawnMobj(thing->x, thing->y, thing->z + thing->height / 2, MT_BLOOD);
            }
            mo->momx = (P_Random() - P_Random()) << 12;
            mo->momy = (P_Random() - P_Random()) << 12;
        }
    }
    return true;
}

boolean P_ChangeSector(sector_t* sector, int crunch)
{
    int x, y;

    nofit = false;
    crushchange = crunch;

    if (sector->special == 666) {
        crushchange = 2;
    }

    for (x = sector->blockbox[BOXLEFT]; x <= sector->blockbox[BOXRIGHT]; x++)
        for (y = sector->blockbox[BOXBOTTOM]; y <= sector->blockbox[BOXTOP]; y++)
            P_BlockThingsIterator(x, y, PIT_ChangeSector);

    return nofit;
}

//
// CHASE CAMERA CLIP/MOVEMENT
//
mobj_t* tmcamera;

static boolean PTR_ChaseCamTraverse(intercept_t* in)
{
    fixed_t x, y, frac;

    if (in->isaline) {
        side_t* side;
        line_t* li = in->d.line;

        if (li->flags & ML_TWOSIDED) {
            if (!(li->flags & (ML_DRAWMASKED | ML_BLOCKING))) {
                sector_t* front;
                sector_t* back;

                P_LineOpening(li);

                front = li->frontsector;
                back = li->backsector;

                if (front->ceilingheight != front->floorheight &&
                    back->ceilingheight != back->floorheight &&
                    back->floorheight    <  front->ceilingheight &&
                    front->ceilingheight >  back->floorheight &&
                    tmcamera->z >= openbottom) {
                    return true;
                }
            }
        }

        side = &sides[li->sidenum[0]];

        if (side->midtexture == 1 || side->toptexture == 1 || side->bottomtexture == 1) {
            return true;
        }

        frac = in->frac - FixedDiv(6 * FRACUNIT, MISSILERANGE);
        x = trace.x + FixedMul(trace.dx, frac);
        y = trace.y + FixedMul(trace.dy, frac);

        tmcamera->x = x;
        tmcamera->y = y;

        return false;
    }
    return true;
}

void P_CheckChaseCamPosition(mobj_t* target, mobj_t* camera, fixed_t x, fixed_t y)
{
    tmcamera = camera;
    if (P_PathTraverse(target->x, target->y, x, y, PT_ADDLINES, PTR_ChaseCamTraverse)) {
        camera->x = x;
        camera->y = y;
    }
}
