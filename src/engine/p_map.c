// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 1993-1997 Id Software, Inc.
// Copyright(C) 1997 Midway Home Entertainment, Inc
// Copyright(C) 2007-2012 Samuel Villarreal
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
// DESCRIPTION:
//      Movement, collision handling.
//      Shooting and aiming.
//
//-----------------------------------------------------------------------------

#include <stdlib.h>

#include "m_fixed.h"
#include "m_random.h"
#include "i_system.h"
#include "m_misc.h"
#include "doomdef.h"
#include "p_local.h"
#include "s_sound.h"
#include "doomstat.h"
#include "r_local.h"
#include "sounds.h"
#include "tables.h"
#include "r_sky.h"
#include "con_console.h"
#include "deh_misc.h"

fixed_t         tmbbox[4];
mobj_t* tmthing;
int             tmflags;
fixed_t         tmx;
fixed_t         tmy;

mobj_t* blockthing;


// If "floatok" true, move would be ok
// if within "tmfloorz - tmceilingz".
int             floatok;

fixed_t         tmfloorz;
fixed_t         tmceilingz;
fixed_t         tmdropoffz;

// keep track of the line that lowers the ceiling,
// so missiles don't explode against sky hack walls
line_t* tmhitline;

// keep track of special lines as they are hit,
// but don't process them until the move is proven valid

line_t* thingspec[MAXTHINGSPEC];
int numthingspec = 0;

//
// P_CheckThingCollision
//

d_inline
static int P_CheckThingCollision(mobj_t* thing) {
    fixed_t blockdist;

    if (netgame && (tmthing->type == MT_PLAYER && thing->type == MT_PLAYER)) {
        return true;    // 20120122 villsa - allow players to go through each other
    }

    blockdist = thing->radius + tmthing->radius;

    fixed_t x, y;
    fixed_t rx, ry;

    x = D_abs(thing->x - tmx);
    y = D_abs(thing->y - tmy);

    rx = blockdist - x;
    ry = blockdist - x;

    if (!(x < y)) {
        if (((rx - y) + (y >> 1)) <= 0) {
            // didn't hit it
            return true;
        }
    }
    else {
        if (((ry - y) + (x >> 1)) <= 0) {
            // didn't hit it
            return true;
        }
    }

    return false;
}

//
// P_BlockMapBox
//

extern byte forcecollision;

d_inline
static void P_BlockMapBox(fixed_t* bbox, fixed_t x, fixed_t y, mobj_t* thing) {
    // [D64] does not use MAXRADIUS

    tmbbox[BOXTOP] = y + thing->radius;
    tmbbox[BOXBOTTOM] = y - thing->radius;
    tmbbox[BOXRIGHT] = x + thing->radius;
    tmbbox[BOXLEFT] = x - thing->radius;

    bbox[BOXLEFT] = (tmbbox[BOXLEFT] - bmaporgx) >> MAPBLOCKSHIFT;
    bbox[BOXRIGHT] = (tmbbox[BOXRIGHT] - bmaporgx) >> MAPBLOCKSHIFT;
    bbox[BOXBOTTOM] = (tmbbox[BOXBOTTOM] - bmaporgy) >> MAPBLOCKSHIFT;
    bbox[BOXTOP] = (tmbbox[BOXTOP] - bmaporgy) >> MAPBLOCKSHIFT;

    if (bbox[BOXLEFT] < 0) {
        bbox[BOXLEFT] = 0;
    }

    if (bbox[BOXRIGHT] < 0) {
        bbox[BOXRIGHT] = 0;
    }

    if (bbox[BOXLEFT] >= bmapwidth) {
        bbox[BOXLEFT] = (bmapwidth - 1);
    }

    if (bbox[BOXRIGHT] >= bmapwidth) {
        bbox[BOXRIGHT] = (bmapwidth - 1);
    }

    if (bbox[BOXBOTTOM] < 0) {
        bbox[BOXBOTTOM] = 0;
    }

    if (bbox[BOXTOP] < 0) {
        bbox[BOXTOP] = 0;
    }

    if (bbox[BOXBOTTOM] >= bmapheight) {
        bbox[BOXBOTTOM] = (bmapheight - 1);
    }

    if (bbox[BOXTOP] >= bmapheight) {
        bbox[BOXTOP] = (bmapheight - 1);
    }
}



//
// MOVEMENT ITERATOR FUNCTIONS
//


//
// PIT_CheckLine
// Adjusts tmfloorz and tmceilingz as lines are contacted
//

boolean PIT_CheckLine(line_t* ld) {
    sector_t* sector;

    if (tmbbox[BOXRIGHT] <= ld->bbox[BOXLEFT]
        || tmbbox[BOXLEFT] >= ld->bbox[BOXRIGHT]
        || tmbbox[BOXTOP] <= ld->bbox[BOXBOTTOM]
        || tmbbox[BOXBOTTOM] >= ld->bbox[BOXTOP]) {
        return true;
    }

    if (P_BoxOnLineSide(tmbbox, ld) != -1) {
        return true;
    }

    // A line has been hit

    // The moving thing's destination position will cross
    // the given line.
    // If this should not be allowed, return false.
    // If the line is special, keep track of it
    // to process later if the move is proven ok.
    // NOTE: specials are NOT sorted by order,
    // so two special lines that are only 8 pixels apart
    // could be crossed in either order.

    if (!ld->backsector) {
        if (tmthing->flags & MF_MISSILE) {
            tmhitline = ld;
        }

        return false;           // one sided line
    }

    if (!(tmthing->flags & MF_MISSILE)) {
        if (ld->flags & ML_BLOCKING) {
            return false;    // explicitly blocking everything
        }

        if (!tmthing->player && ld->flags & ML_BLOCKMONSTERS) {
            return false;    // block monsters only
        }
    }

    // [d64] don't cross mid-pegged lines
    if (ld->flags & ML_BLOCKPROJECTILES) {
        tmhitline = ld;
        return false;
    }

    // [kex] check if thing's midpoint is inside sector
    if (tmthing->blockflag & BF_MIDPOINTONLY && ld->backsector) {
        if (tmthing->subsector->sector != ld->backsector) {
            return true;
        }
    }

    sector = ld->frontsector;

    // [d64] check for valid sector heights
    if (sector->ceilingheight == sector->floorheight) {
        tmhitline = ld;
        return false;
    }

    sector = ld->backsector;

    // [d64] check for valid sector heights
    if (sector->ceilingheight == sector->floorheight) {
        tmhitline = ld;
        return false;
    }

    // set openrange, opentop, openbottom
    P_LineOpening(ld);

    // adjust floor / ceiling heights
    if (opentop < tmceilingz) {
        tmceilingz = opentop;
        tmhitline = ld;
    }

    if (openbottom > tmfloorz) {
        tmfloorz = openbottom;
    }

    if (lowfloor < tmdropoffz) {
        tmdropoffz = lowfloor;
    }

    // if contacted a special line, add it to the list
    if (ld->special & MLU_CROSS) {
        if (numthingspec >= MAXSPECIALCROSS) {
            CON_Warnf("PIT_CheckLine: spechit overflow!\n");
        }
        else {
            thingspec[numthingspec] = ld;
            numthingspec++;
        }
    }

    return true;
}

//
// PIT_CheckThing
//

boolean PIT_CheckThing(mobj_t* thing) {
    int    solid;

    if (!(thing->flags & (MF_SOLID | MF_SPECIAL | MF_SHOOTABLE))) {
        return true;
    }

    if (P_CheckThingCollision(thing)) {
        return true;
    }

    // don't clip against self
    if (thing == tmthing) {
        return true;
    }

    if (tmthing->blockflag & BF_MOBJPASS) {
        if (tmthing->z >= thing->z + thing->height && !(thing->flags & MF_SPECIAL)) {
            return true;
        }
        else if (tmthing->z + tmthing->height < thing->z && !(thing->flags & MF_SPECIAL)) {
            return true;
        }
    }

    // skull slammed into somthing...
    if (tmthing->flags & MF_SKULLFLY) {
        blockthing = thing;
        return false;
    }

    // missiles can hit other things
    if (tmthing->flags & MF_MISSILE) {
        // see if it went over / under
        if (tmthing->z > thing->z + thing->height) {
            return true;    // overhead
        }
        if (tmthing->z + tmthing->height < thing->z) {
            return true;    // underneath
        }

        if (tmthing->target && tmthing->target->type == thing->type) {
            // Don't hit same species as originator.
            if (thing == tmthing->target) {
                return true;

                // sdh: Add deh_species_infighting here.  We can override the
                // "monsters of the same species cant hurt each other" behavior
                // through dehacked patches

                if (thing->type != MT_PLAYER && !deh_species_infighting)
                {
                    // Explode, but do no damage.
                    // Let players missile other players.
                    return false;
                }
            }

            // Explode, but do no damage.
            // Let players missile other players.
            if (thing->type != MT_PLAYER) {
                return false;
            }
        }

        if (!(thing->flags & MF_SHOOTABLE)) {
            // didn't do any damage
            return !(thing->flags & MF_SOLID);
        }

        blockthing = thing;

        // don't traverse any more
        return false;
    }

    solid = thing->flags & MF_SOLID;

    // check for special pickup
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

//
// P_CheckPosition
// This is purely informative, nothing is modified
// (except things picked up).
//
// in:
//  a mobj_t (can be valid or invalid)
//  a position to be checked
//   (doesn't need to be related to the mobj_t->x,y)
//
// during:
//  special things are touched if MF_PICKUP
//  early out on solid lines?
//
// out:
//  newsubsec
//  floorz
//  ceilingz
//  tmdropoffz
//   the lowest point contacted
//   (monsters won't move to a dropoff)
//  speciallines[]
//  numspeciallines
//
boolean P_CheckPosition(mobj_t* thing, fixed_t x, fixed_t y) {
    int             bx;
    int             by;
    subsector_t* newsubsec;
    fixed_t         bbox[4];

    tmthing = thing;
    tmflags = thing->flags;

    tmx = x;
    tmy = y;

    newsubsec = R_PointInSubsector(x, y);

    tmhitline = NULL;
    blockthing = NULL;

    // The base floor / ceiling is from the subsector
    // that contains the point.
    // Any contacted lines the step closer together
    // will adjust them.
    tmfloorz = tmdropoffz = newsubsec->sector->floorheight;
    tmceilingz = newsubsec->sector->ceilingheight;

    validcount++;
    numthingspec = 0;

    if (tmflags & MF_NOCLIP) {
        return true;
    }

    // Check things first, possibly picking things up.
    // [d64] MAXRADIUS is not used
    P_BlockMapBox(bbox, x, y, tmthing);

    for (bx = bbox[BOXLEFT]; bx <= bbox[BOXRIGHT]; bx++) {
        for (by = bbox[BOXBOTTOM]; by <= bbox[BOXTOP]; by++) {
            if (!P_BlockThingsIterator(bx, by, PIT_CheckThing)) {
                return false;
            }
        }
    }

    // check lines
    for (bx = bbox[BOXLEFT]; bx <= bbox[BOXRIGHT]; bx++) {
        for (by = bbox[BOXBOTTOM]; by <= bbox[BOXTOP]; by++) {
            if (!P_BlockLinesIterator(bx, by, PIT_CheckLine)) {
                return false;
            }
        }
    }

    return true;
}


//
// P_TryMove
//
// Attempt to move to a new position,
// crossing special lines unless MF_TELEPORT is set.
//

boolean P_TryMove(mobj_t* thing, fixed_t x, fixed_t y) {
    fixed_t     oldx;
    fixed_t     oldy;
    int         side;
    int         oldside;
    line_t* ld;

    floatok = false;
    if (!P_CheckPosition(thing, x, y)) {
        return false;    // solid wall or thing
    }

    if (!(thing->flags & MF_NOCLIP)) {
        if (tmceilingz - tmfloorz < thing->height) {
            return false;    // doesn't fit
        }

        floatok = true;

        if (!(thing->flags & MF_TELEPORT)
            && tmceilingz - thing->z < thing->height) {
            return false;    // mobj must lower itself to fit
        }

        if (!(thing->flags & MF_TELEPORT)
            && tmfloorz - thing->z > 24 * FRACUNIT) {
            return false;    // too big a step up
        }

        if (!(thing->flags & (MF_DROPOFF | MF_FLOAT))
            && tmfloorz - tmdropoffz > 24 * FRACUNIT) {
            return false;    // don't stand over a dropoff
        }
    }

    // the move is ok,
    // so link the thing into its new position
    P_UnsetThingPosition(thing);

    oldx = thing->x;
    oldy = thing->y;
    thing->floorz = tmfloorz;
    thing->ceilingz = tmceilingz;
    thing->x = x;
    thing->y = y;

    P_SetThingPosition(thing);

    // if any special lines were hit, do the effect
    if (!(thing->flags & (MF_TELEPORT | MF_NOCLIP))) {
        while (numthingspec--) {
            // see if the line was crossed
            ld = thingspec[numthingspec];
            side = P_PointOnLineSide(thing->x, thing->y, ld);
            oldside = P_PointOnLineSide(oldx, oldy, ld);
            if (side != oldside) {
                if (!(ld->flags & ML_TRIGGERFRONT) || (side))
                {
                    P_UseSpecialLine(thing, ld, oldside);
                }
            }
        }
    }

    return true;
}

//
// P_PlayerMove
//
// Alternate P_TryMove function for players which
// handles pickups. Needed in order to mimic
// item grabbing behavior
//

int P_PlayerMove(mobj_t* thing, fixed_t x, fixed_t y) {
    int moveok;

    moveok = P_TryMove(thing, x, y);

    //
    // [d64] seems more like a console optimization but now the player
    // is no longer able to pick up items during the block things iteration.
    // this means that only one item can be picked up per tic.
    //
    if (thing->flags & MF_PICKUP && blockthing) {
        mobj_t* touchThing = blockthing;

        if (touchThing->flags & MF_SPECIAL) {
            // can remove thing
            P_TouchSpecialThing(touchThing, thing);
            blockthing = NULL;
        }
    }

    return moveok;
}

//
// TELEPORT MOVE
//

//
// P_TeleportMove
//

boolean P_TeleportMove(mobj_t* thing, fixed_t x, fixed_t y) {
    int             bx;
    int             by;
    subsector_t* newsubsec;
    fixed_t         bbox[4];

    tmthing = thing;
    tmflags = thing->flags;

    tmx = x;
    tmy = y;

    newsubsec = R_PointInSubsector(x, y);
    tmhitline = NULL;

    // The base floor/ceiling is from the subsector
    // that contains the point.
    // Any contacted lines the step closer together
    // will adjust them.
    tmfloorz = tmdropoffz = newsubsec->sector->floorheight;
    tmceilingz = newsubsec->sector->ceilingheight;

    validcount++;
    numthingspec = 0;

    P_BlockMapBox(bbox, x, y, tmthing);

    for (bx = bbox[BOXLEFT]; bx <= bbox[BOXRIGHT]; bx++) {
        for (by = bbox[BOXBOTTOM]; by <= bbox[BOXTOP]; by++) {
            // [d64] do stomping in actual teleport function
            if (!P_BlockThingsIterator(bx, by, PIT_CheckThing)) {
                return false;
            }
        }
    }

    // the move is ok,
    // so link the thing into its new position
    P_UnsetThingPosition(thing);

    thing->floorz = tmfloorz;
    thing->ceilingz = tmceilingz;
    thing->x = x;
    thing->y = y;

    P_SetThingPosition(thing);

    return true;
}


//
// P_ThingHeightClip
// Takes a valid thing and adjusts the thing->floorz,
// thing->ceilingz, and possibly thing->z.
// This is called for all nearby monsters
// whenever a sector changes height.
// If the thing doesn't fit,
// the z will be set to the lowest value
// and false will be returned.
//

static int P_ThingHeightClip(mobj_t* thing) {
    int            onfloor;

    onfloor = (thing->z == thing->floorz);

    P_CheckPosition(thing, thing->x, thing->y);
    // what about stranding a monster partially off an edge?

    thing->floorz = tmfloorz;
    thing->ceilingz = tmceilingz;

    if (onfloor) {
        thing->z = thing->floorz;
    }
    else {
        // don't adjust a floating monster unless forced to
        if (thing->z + thing->height > thing->ceilingz) {
            thing->z = thing->ceilingz - thing->height;
        }
    }

    if (thing->ceilingz - thing->floorz < thing->height) {
        return false;
    }

    return true;
}

//
// PIT_CheckMobjZ
//

mobj_t* onmobj;

static boolean PIT_CheckMobjZ(mobj_t* thing) {
    if (!(thing->flags & (MF_SOLID | MF_SPECIAL | MF_SHOOTABLE))) {
        return true;    // Can't hit thing
    }

    if (P_CheckThingCollision(thing)) {
        return true;
    }

    if (thing == tmthing) {
        return true;    // Don't clip against self
    }

    if (tmthing->z > thing->z + thing->height) {
        return true;
    }

    else if (tmthing->z + tmthing->height < thing->z) {
        return true;    // under thing
    }

    if (thing->flags & MF_SOLID) {
        onmobj = thing;
    }

    return(!(thing->flags & MF_SOLID));
}

//
// P_CheckOnMobj
//

void P_ZMovement(mobj_t* mo, int checkmissile);

mobj_t* P_CheckOnMobj(mobj_t* thing) {
    int    bx, by;
    subsector_t* newsubsec;
    fixed_t x;
    fixed_t y;
    mobj_t oldmo;
    fixed_t bbox[4];

    x = thing->x;
    y = thing->y;
    tmthing = thing;
    tmflags = thing->flags;
    oldmo = *thing; // save the old mobj before the fake zmovement
    P_ZMovement(tmthing, false);

    tmx = x;
    tmy = y;

    newsubsec = R_PointInSubsector(x, y);
    tmhitline = NULL;

    //
    // the base floor / ceiling is from the subsector that contains the
    // point.  Any contacted lines the step closer together will adjust them
    //
    tmfloorz = tmdropoffz = newsubsec->sector->floorheight;
    tmceilingz = newsubsec->sector->ceilingheight;

    validcount++;
    numthingspec = 0;

    if (tmflags & MF_NOCLIP) {
        return NULL;
    }

    //
    // check things first, possibly picking things up
    // the bounding box is extended by MAXRADIUS because mobj_ts are grouped
    // into mapblocks based on their origin point, and can overlap into adjacent
    // blocks by up to MAXRADIUS units
    //
    // [d64]: intentionally removed MAXRADIUS?
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
// Allows the player to slide along any angled walls.
//

fixed_t         bestslidefrac;
line_t* bestslideline;

mobj_t* slidemo;

fixed_t         tmxmove;
fixed_t         tmymove;


//
// PTR_SlideTraverse
//

boolean PTR_SlideTraverse(intercept_t* in) {
    line_t* li = in->d.line;

    if (!(li->flags & ML_TWOSIDED)) {
        if (P_PointOnLineSide(slidemo->x, slidemo->y, li)) {
            // don't hit the back side
            return true;
        }

        goto isblocking;
    }

    // set openrange, opentop, openbottom
    P_LineOpening(li);

    if (openrange < slidemo->height) {
        goto isblocking;    // doesn't fit
    }

    if (opentop - slidemo->z < slidemo->height) {
        goto isblocking;    // mobj is too high
    }

    if (openbottom - slidemo->z > 24 * FRACUNIT) {
        goto isblocking;    // too big a step up
    }

    // this line doesn't block movement
    return true;

    // the line does block movement,
    // see if it is closer than best so far

isblocking:

    if (in->frac < bestslidefrac) {
        bestslidefrac = in->frac;
        bestslideline = li;
    }

    return false;       // stop
}

//
// P_SlideMove
// The momx / momy move is bad, so try to slide
// along a wall.
// Find the first line hit, move flush to it,
// and slide along it
//
// This is a kludgy mess.
//

void P_SlideMove(mobj_t* mo) {
    fixed_t leadx;
    fixed_t leady;
    fixed_t trailx;
    fixed_t traily;
    fixed_t newx;
    fixed_t newy;
    int     hitcount;
    line_t* ld;
    int     an1;
    int     an2;

    slidemo = mo;
    hitcount = 0;

retry:
    if (++hitcount == 3) {
        goto stairstep;    // don't loop forever
    }


    // trace along the three leading corners
    if (mo->momx > 0) {
        leadx = mo->x + mo->radius;
        trailx = mo->x - mo->radius;
    }
    else {
        leadx = mo->x - mo->radius;
        trailx = mo->x + mo->radius;
    }

    if (mo->momy > 0) {
        leady = mo->y + mo->radius;
        traily = mo->y - mo->radius;
    }
    else {
        leady = mo->y - mo->radius;
        traily = mo->y + mo->radius;
    }

    bestslidefrac = FRACUNIT + 1;

    P_PathTraverse(leadx, leady, leadx + mo->momx, leady + mo->momy,
        PT_ADDLINES, PTR_SlideTraverse);
    P_PathTraverse(trailx, leady, trailx + mo->momx, leady + mo->momy,
        PT_ADDLINES, PTR_SlideTraverse);
    P_PathTraverse(leadx, traily, leadx + mo->momx, traily + mo->momy,
        PT_ADDLINES, PTR_SlideTraverse);

    // move up to the wall
    if (bestslidefrac == FRACUNIT + 1) {
        // the move most have hit the middle, so stairstep
    stairstep:
        if (!P_PlayerMove(mo, mo->x, mo->y + mo->momy)) {
            if (!P_PlayerMove(mo, mo->x + mo->momx, mo->y)) {
                // [d64] set momx and momy to 0
                mo->momx = 0;
                mo->momy = 0;
            }
        }

        return;
    }

    // fudge a bit to make sure it doesn't hit
    bestslidefrac -= 0x800;
    if (bestslidefrac > 0) {
        newx = FixedMul(mo->momx, bestslidefrac);
        newy = FixedMul(mo->momy, bestslidefrac);

        if (!P_PlayerMove(mo, mo->x + newx, mo->y + newy)) {
            bestslidefrac = FRACUNIT;

            // [d64] jump to hitslideline instead of stairstep
            goto hitslideline;
        }
    }

    // Now continue along the wall.
    // First calculate remainder.
    bestslidefrac = FRACUNIT - (bestslidefrac + 0x800);

    if (bestslidefrac > FRACUNIT) {
        bestslidefrac = FRACUNIT;
    }

    if (bestslidefrac <= 0) {
        return;
    }

    //
    // [d64] code below is loosely based on P_HitSlideLine
    //
hitslideline:

    ld = bestslideline;

    if (ld->slopetype == ST_HORIZONTAL) {
        tmymove = 0;
    }
    else {
        tmymove = FixedMul(mo->momy, bestslidefrac);
    }

    if (ld->slopetype == ST_VERTICAL) {
        tmxmove = 0;
    }
    else {
        tmxmove = FixedMul(mo->momx, bestslidefrac);
    }

    //
    // [d64] this new algorithm seems to reduce the chances
    // of boosting the player's speed when wall running
    //

    an1 = finecosine[ld->angle];
    an2 = finesine[ld->angle];

    if (P_PointOnLineSide(mo->x, mo->y, bestslideline)) {
        //
        // [d64] same as deltaangle += ANG180 ?
        //
        an1 = -an1;
        an2 = -an2;
    }

    newx = FixedMul(tmxmove, an1);
    newy = FixedMul(tmymove, an2);

    mo->momx = FixedMul(newx + newy, an1);
    mo->momy = FixedMul(newx + newy, an2);

    if (!P_PlayerMove(mo, mo->x + mo->momx, mo->y + mo->momy)) {
        goto retry;
    }
}


//
// P_LineAttack
//
mobj_t* linetarget;     // who got hit (or NULL)
mobj_t* shootthing;
//fixed_t         aimfrac;

fixed_t         shootdirx;
fixed_t         shootdiry;
fixed_t         shootdirz;

//int        shotsideline = false;
//int        aimlaser = false;

// Height if not aiming up or down
// ???: use slope for monsters?
fixed_t         shootz;

int             la_damage;
fixed_t         attackrange;

fixed_t         aimslope;
fixed_t         aimpitch;

// slopes to top and bottom of target
extern fixed_t  topslope;
extern fixed_t  bottomslope;

extern fixed_t laserhit_x;
extern fixed_t laserhit_y;
extern fixed_t laserhit_z;

//
// PTR_AimTraverse
// Sets linetaget and aimslope when a target is aimed at.
//
boolean PTR_AimTraverse(intercept_t* in) {
    line_t* li;
    mobj_t* th;
    fixed_t linebottomslope;
    fixed_t linetopslope;
    fixed_t thingtopslope;
    fixed_t thingbottomslope;
    fixed_t dist;

    if (in->isaline) {
        li = in->d.line;

        if (!(li->flags & ML_TWOSIDED)) {
            return false;    // stop
        }

        // Crosses a two sided line.
        // A two sided line will restrict
        // the possible target ranges.
        P_LineOpening(li);

        if (openbottom >= opentop) {
            return false;    // stop
        }

        dist = FixedMul(attackrange, in->frac);

        if (li->frontsector->floorheight != li->backsector->floorheight) {
            linebottomslope = FixedDiv(openbottom - shootz, dist);
            if (linebottomslope > bottomslope) {
                bottomslope = linebottomslope;
            }
        }

        if (li->frontsector->ceilingheight != li->backsector->ceilingheight) {
            linetopslope = FixedDiv(opentop - shootz, dist);
            if (linetopslope < topslope) {
                topslope = linetopslope;
            }
        }

        if (topslope <= bottomslope) {
            return false;    // stop
        }

        return true;                    // shot continues
    }

    // shoot a thing
    th = in->d.thing;
    if (th == shootthing) {
        return true;    // can't shoot self
    }

    if (!(th->flags & MF_SHOOTABLE)) {
        return true;    // corpse or something
    }

    // check angles to see if the thing can be aimed at
    dist = FixedMul(attackrange, in->frac);
    thingtopslope = FixedDiv(th->z + th->height - shootz, dist);

    if (thingtopslope < bottomslope) {
        return true;    // shot over the thing
    }

    thingbottomslope = FixedDiv(th->z - shootz, dist);

    if (thingbottomslope > topslope) {
        return true;    // shot under the thing
    }

    // this thing can be hit!
    if (thingtopslope > topslope) {
        thingtopslope = topslope;
    }

    if (thingbottomslope < bottomslope) {
        thingbottomslope = bottomslope;
    }

    aimslope = (thingtopslope + thingbottomslope) >> 1;
    linetarget = th;
    //aimfrac = in->frac;

    return false;                       // don't go any farther
}

//
// PTR_ShootTraverse
//
boolean PTR_ShootTraverse(intercept_t* in) {
    fixed_t     x;
    fixed_t     y;
    fixed_t     z;
    fixed_t     frac;
    line_t* li;
    mobj_t* th;
    fixed_t     slope;
    fixed_t     dist;
    fixed_t     thingtopslope;
    fixed_t     thingbottomslope;
    int    hitplane = false;
    int         lineside;
    sector_t* sidesector;
    fixed_t     hitz;

    x = y = z = 0;

    if (in->isaline) {
        li = in->d.line;

        lineside = P_PointOnLineSide(shootthing->x, shootthing->y, li);

        // [d64] villsa 02252014: moved here
        if (li->special & MLU_SHOOT) {
            P_UseSpecialLine(shootthing, li, lineside);
        }

        sidesector = lineside ? li->backsector : li->frontsector;
        dist = FixedMul(attackrange, in->frac);
        hitz = shootz + FixedMul(dcos(shootthing->pitch - ANG90), dist);

        if (li->flags & ML_TWOSIDED && li->backsector) {
            // crosses a two sided line
            P_LineOpening(li);

            if ((li->frontsector->floorheight == li->backsector->floorheight
                || (slope = FixedDiv(openbottom - shootz, dist)) <= aimslope)
                && (li->frontsector->ceilingheight == li->backsector->ceilingheight
                    || (slope = FixedDiv(opentop - shootz, dist)) >= aimslope)) {
                return true;    // shot continues
            }
        }

        if (sidesector != NULL) {
            if (!(hitz > sidesector->floorheight && hitz < sidesector->ceilingheight)) {
                hitplane = true;
            }
        }

        //
        // [kex] - hit ceiling/floor
        // set position based on intersection
        //
        if (hitplane == true) {
            plane_t* plane;
            fixed_t den;

            if (hitz <= sidesector->floorheight) {
                plane = &sidesector->floorplane;
            }
            else {
                plane = &sidesector->ceilingplane;
            }

            den = FixedDot(plane->a, plane->b, plane->c, shootdirx, shootdiry, shootdirz);

            if (den != 0) {
                fixed_t hitdist;
                fixed_t num = FixedDot(
                    plane->a, plane->b,
                    plane->c, trace.x,
                    trace.y, shootz) + plane->d;

                hitdist = FixedDiv(-num, den);

                frac = FixedDiv(hitdist, attackrange);
                x = trace.x + FixedMul(FixedMul(aimpitch, trace.dx), frac);
                y = trace.y + FixedMul(FixedMul(aimpitch, trace.dy), frac);
                z = hitz;
            }
        }
        else {
            // hit line
            // position a bit closer
            frac = in->frac - FixedDiv(4 * FRACUNIT, attackrange);
            x = trace.x + FixedMul(trace.dx, frac);
            y = trace.y + FixedMul(trace.dy, frac);
            z = shootz + FixedMul(aimslope, FixedMul(frac, attackrange));
        }

        //don't shoot blank textures
        if (sides[li->sidenum[0]].midtexture == 1
            || sides[li->sidenum[0]].toptexture == 1
            || sides[li->sidenum[0]].bottomtexture == 1) {
            return false;
        }

        if (li->frontsector->ceilingpic == skyflatnum) {
            // don't shoot the sky!
            if (z > li->frontsector->ceilingheight) {
                return false;
            }

            // it's a sky hack wall
            if (li->backsector && li->backsector->ceilingpic == skyflatnum
                && li->backsector->ceilingheight < z) {
                return false;
            }
        }

        if (attackrange == LASERRANGE) {
            laserhit_x = x;
            laserhit_y = y;
            laserhit_z = z;
        }
        else {
            // Spawn bullet puffs.
            P_SpawnPuff(x, y, z);
        }

        // don't go any farther
        return false;
    }

    // shoot a thing
    th = in->d.thing;
    if (th == shootthing) {
        return true;    // can't shoot self
    }

    if (!(th->flags & MF_SHOOTABLE)) {
        return true;    // corpse or something
    }

    // check angles to see if the thing can be aimed at
    dist = FixedMul(attackrange, in->frac);
    thingtopslope = FixedDiv(th->z + th->height - shootz, dist);

    if (thingtopslope < aimslope) {
        return true;    // shot over the thing
    }

    thingbottomslope = FixedDiv(th->z - shootz, dist);

    if (thingbottomslope > aimslope) {
        return true;    // shot under the thing
    }


    // hit thing
    // position a bit closer
    frac = in->frac - FixedDiv(10 * FRACUNIT, attackrange);

    x = trace.x + FixedMul(trace.dx, frac);
    y = trace.y + FixedMul(trace.dy, frac);
    z = shootz + FixedMul(aimslope, FixedMul(frac, attackrange));

    // Spawn bullet puffs or blod spots,
    // depending on target type.
    //
    // [kex] if something was hit with the laser then save the
    // hit location for later
    //
    if (attackrange == LASERRANGE) {
        laserhit_x = x;
        laserhit_y = y;
        laserhit_z = z;
    }
    else if (in->d.thing->flags & MF_NOBLOOD) {
        P_SpawnPuff(x, y, z);
    }
    else {
        P_SpawnBlood(x, y, z, la_damage);
    }

    if (la_damage) {
        P_DamageMobj(th, shootthing, shootthing, la_damage);
    }

    // don't go any farther
    return false;

}


//
// P_AimLineAttack
//

fixed_t P_AimLineAttack(mobj_t* t1, angle_t angle, fixed_t zheight, fixed_t distance) {
    int flags;
    fixed_t x2;
    fixed_t y2;
    fixed_t pitch = 0;
    fixed_t dist;

    angle >>= ANGLETOFINESHIFT;
    dist = distance >> FRACBITS;

    shootthing = t1;

    x2 = t1->x + dist * finecosine[angle];
    y2 = t1->y + dist * finesine[angle];

    // can't shoot outside view angles
    // [d64] use 120 instead of 100
    topslope = 120 * FRACUNIT / 160;
    bottomslope = -120 * FRACUNIT / 160;

    attackrange = distance;
    linetarget = NULL;
    flags = PT_ADDLINES | PT_ADDTHINGS | PT_EARLYOUT;

    // [d64] new argument for shoot height
    if (!zheight)
    {
        shootz = t1->z + (t1->height >> 1) + 12 * FRACUNIT;
    }
    else
    {
        shootz = t1->z + zheight;
    }

    if (t1->player) {
        pitch = dcos(D_abs(t1->pitch - ANG90));

        // [kex] check for autoaim option. skip if bfg spray
        if (distance != (ATTACKRANGE + 1)) {
            if (!t1->player->autoaim &&
                distance != MELEERANGE && distance != (MELEERANGE + 1)) {
                flags &= ~PT_ADDTHINGS;
            }
        }
    }

    P_PathTraverse(t1->x, t1->y, x2, y2, flags, PTR_AimTraverse);

    if (linetarget) {
        return aimslope;
    }

    // [kex] adjust slope based on view pitch
    else if (t1->player) {
        aimslope = pitch;

        if (D_abs(aimslope) > 0x40) {
            return aimslope;
        }
    }

    return 0;
}


//
// P_LineAttack
// If damage == 0, it is just a test trace
// that will leave linetarget set.
//

void P_LineAttack(mobj_t* t1, angle_t angle, fixed_t distance, fixed_t slope, int damage) {
    fixed_t x2;
    fixed_t y2;
    fixed_t dist;

    angle >>= ANGLETOFINESHIFT;
    dist = distance >> FRACBITS;

    shootthing = t1;
    la_damage = damage;

    x2 = t1->x + dist * finecosine[angle];
    y2 = t1->y + dist * finesine[angle];

    shootz = t1->z + (t1->height >> 1) + 12 * FRACUNIT; // [d64] changed from 8 to 12

    attackrange = distance;
    aimslope = slope;
    aimpitch = dcos(shootthing->pitch);

    //
    // [kex] - stuff for plane hit detection
    //
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

static mobj_t* usething = NULL;
static int     usecontext = false;
static int     displaycontext = false;
line_t* contextline = NULL;

//
// P_CheckUseHeight
//

static int P_CheckUseHeight(line_t* line, mobj_t* thing) {
    fixed_t check = 0;

    if (!(line->flags & ML_SWITCHX02 ||
        line->flags & ML_SWITCHX04 ||
        line->flags & ML_SWITCHX08)) {
        return true;    // ignore non-switches
    }

    if (line->flags & ML_CHECKFLOORHEIGHT) {
        if (line->flags & ML_TWOSIDED) {
            check = (line->backsector->floorheight + sides[line->sidenum[0]].rowoffset) - (32 * FRACUNIT);
        }
        else {
            check = (line->frontsector->floorheight + sides[line->sidenum[0]].rowoffset) + (32 * FRACUNIT);
        }
    }
    else if (line->flags & ML_TWOSIDED) {
        check = (line->backsector->ceilingheight + sides[line->sidenum[0]].rowoffset) + (32 * FRACUNIT);
    }
    else {
        return true;
    }

    if (!(check < thing->z)) {
        if ((thing->z + thing->height) < check) {
            return false;
        }
    }
    else {
        return false;
    }

    return true;
}

//
// PTR_UseTraverse
//

boolean PTR_UseTraverse(intercept_t* in) {
    if (!in->d.line->special) {
        P_LineOpening(in->d.line);

        if (openrange <= 0) {
            //
            // [kex] don't spam oof sound if usecontext is on
            //
            if (!usecontext) {
                S_StartSound(usething, sfx_noway);
            }

            return false;    // can't use through a wall
        }


        return true;    // keep checking
    }

    if (P_PointOnLineSide(usething->x, usething->y, in->d.line) == 1) {
        return true;
    }

    if (!(in->d.line->special & MLU_USE) || !P_CheckUseHeight(in->d.line, usething)) {
        if (!usecontext) {
            S_StartSound(usething, sfx_noway);
        }
        return false;
    }

    //
    // [kex] don't trigger special if usecontext is on
    //
    if (!usecontext) {
        P_UseSpecialLine(usething, in->d.line, 0);
    }

    displaycontext = true;
    contextline = in->d.line;

    // can't use for than one special line in a row
    return false;
}

//
// P_UseLines
// Looks for special lines in front of the player to activate.
//

boolean P_UseLines(player_t* player, int showcontext) {
    int        angle;
    fixed_t    x1;
    fixed_t    y1;
    fixed_t    x2;
    fixed_t    y2;

    usething = player->mo;

    //
    // [kex] use context stuff
    //
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
int             bombdamage;


//
// PIT_RadiusAttack
// "bombsource" is the creature that caused the explosion at "bombspot"
//

boolean PIT_RadiusAttack(mobj_t* thing) {
    fixed_t     dx;
    fixed_t     dy;
    fixed_t     dist;

    if (!(thing->flags & MF_SHOOTABLE)) {
        return true;
    }

    // Boss cyborg take no damage from concussion.
    if (thing->type == MT_CYBORG || thing->type == MT_SPIDER) {
        return true;
    }

    if (thing->type == MT_SKULL || thing->type == MT_PAIN) {
        if (bombsource && bombsource->type == MT_SKULL) {
            return true;
        }
    }

    dx = D_abs(thing->x - bombspot->x);
    dy = D_abs(thing->y - bombspot->y);

    dist = dx > dy ? dx : dy;
    dist = F2INT(dist - thing->radius);

    if (dist < 0) {
        dist = 0;
    }

    if (dist >= bombdamage) {
        return true;    // out of range
    }

    if (P_CheckSight(thing, bombspot) != 0) // must be in direct path */
    {
        P_DamageMobj(thing, bombspot, bombsource, bombdamage - dist);
    }

    return true;
}


//
// P_RadiusAttack
// Source is the creature that caused the explosion at spot.
//
void P_RadiusAttack(mobj_t* spot, mobj_t* source, int damage) {
    int         x;
    int         y;

    int         xl;
    int         xh;
    int         yl;
    int         yh;

    fixed_t     dist;

    dist = (damage + MAXRADIUS) << FRACBITS;
    yh = (spot->y + dist - bmaporgy) >> MAPBLOCKSHIFT;
    yl = (spot->y - dist - bmaporgy) >> MAPBLOCKSHIFT;
    xh = (spot->x + dist - bmaporgx) >> MAPBLOCKSHIFT;
    xl = (spot->x - dist - bmaporgx) >> MAPBLOCKSHIFT;
    bombspot = spot;
    bombsource = source;
    bombdamage = damage;

    for (y = yl; y <= yh; y++)
        for (x = xl; x <= xh; x++) {
            P_BlockThingsIterator(x, y, PIT_RadiusAttack);
        }
}



//
// SECTOR HEIGHT CHANGING
// After modifying a sectors floor or ceiling height,
// call this routine to adjust the positions
// of all things that touch the sector.
//
// If anything doesn't fit anymore, true will be returned.
// If crunch is true, they will take damage
//  as they are being crushed.
// If Crunch is false, you should set the sector height back
//  the way it was and call P_ChangeSector again
//  to undo the changes.
//
static int crushchange;
static int nofit;


//
// PIT_ChangeSector
//
boolean PIT_ChangeSector(mobj_t* thing) {
    mobj_t* mo;

    if (P_ThingHeightClip(thing)) {
        // keep checking
        return true;
    }

    // crunch bodies to giblets
    if (thing->health <= 0 && thing->tics == -1) {
        if (thing->state != &states[S_CORPSE]) {
            P_SetMobjState(thing, S_CORPSE);
            S_StartSound(thing, sfx_slop);
        }

        // keep checking
        return true;
    }

    // crunch dropped items
    if (thing->flags & MF_DROPPED) {
        P_RemoveMobj(thing);

        // keep checking
        return true;
    }

    if (!(thing->flags & MF_SHOOTABLE)) {
        // assume it is bloody gibs or something
        return true;
    }

    nofit = true;

    //
    // [d64] insta-kill thing if sector type is 666
    //
    if (crushchange == 2) {
        P_DamageMobj(thing, NULL, NULL, 99999);
        return true;
    }
    else {
        if (crushchange && !(leveltime & 3)) {
            P_DamageMobj(thing, NULL, NULL, 10);

            // spray blood in a random direction
            mo = P_SpawnMobj(thing->x, thing->y, thing->z + thing->height / 2, MT_BLOOD);
            mo->momx = (P_Random() - P_Random()) << 12;
            mo->momy = (P_Random() - P_Random()) << 12;
        }
    }

    // keep checking (crush other things)
    return true;
}



//
// P_ChangeSector
//
boolean P_ChangeSector(sector_t* sector, int crunch) {
    int         x;
    int         y;

    nofit = false;
    crushchange = crunch;

    // [d64] handle special case if sector's special is 666
    if (sector->special == 666) {
        crushchange = 2;
    }

    // re-check heights for all things near the moving sector
    for (x = sector->blockbox[BOXLEFT]; x <= sector->blockbox[BOXRIGHT]; x++)
        for (y = sector->blockbox[BOXBOTTOM]; y <= sector->blockbox[BOXTOP]; y++) {
            P_BlockThingsIterator(x, y, PIT_ChangeSector);
        }

    return nofit;
}

//
// CHASE CAMERA CLIP/MOVEMENT
//

mobj_t* tmcamera;

//
// PTR_ChaseCamTraverse
//

static boolean PTR_ChaseCamTraverse(intercept_t* in) {
    fixed_t x;
    fixed_t y;
    fixed_t frac;

    if (in->isaline) {
        side_t* side;
        line_t* li;

        li = in->d.line;

        if (li->flags & ML_TWOSIDED) {
            if (!(li->flags & (ML_DRAWMASKED | ML_BLOCKING))) {
                sector_t* front;
                sector_t* back;

                P_LineOpening(li);

                front = li->frontsector;
                back = li->backsector;

                if (front->ceilingheight != front->floorheight &&
                    back->ceilingheight != back->floorheight &&
                    back->floorheight < front->ceilingheight &&
                    front->ceilingheight > back->floorheight &&
                    tmcamera->z >= openbottom) {
                    return true;    // continue
                }
            }
        }

        side = &sides[li->sidenum[0]];

        if (side->midtexture == 1 || side->toptexture == 1 || side->bottomtexture == 1) {
            return true;    // continue
        }

        frac = in->frac - FixedDiv(6 * FRACUNIT, MISSILERANGE);
        x = trace.x + FixedMul(trace.dx, frac);
        y = trace.y + FixedMul(trace.dy, frac);

        tmcamera->x = x;
        tmcamera->y = y;

        return false; // don't go any farther
    }

    return true;
}

//
// P_CheckChaseCamPosition
//

void P_CheckChaseCamPosition(mobj_t* target, mobj_t* camera, fixed_t x, fixed_t y) {
    tmcamera = camera;

    if (P_PathTraverse(target->x, target->y, x, y, PT_ADDLINES, PTR_ChaseCamTraverse)) {
        camera->x = x;
        camera->y = y;
    }
}
