// Emacs style mode select   -*- C -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 1993-1997 Id Software, Inc.
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

#ifndef __P_LOCAL__
#define __P_LOCAL__

#include "doomdef.h"
#include "d_player.h"
#include "w_wad.h"
#include "info.h"
#include "m_menu.h"
#include "t_bsp.h"

#define FLOATSPEED      (FRACUNIT*4)

#define MAXHEALTH       100
#define	VIEWHEIGHT		(56*FRACUNIT) //  D64 change to 41

// mapblocks are used to check movement
// against lines and things
#define MAPBLOCKUNITS   128
#define MAPBLOCKSIZE    (MAPBLOCKUNITS*FRACUNIT)
#define MAPBLOCKSHIFT   (FRACBITS+7)
#define MAPBMASK        (MAPBLOCKSIZE-1)
#define MAPBTOFRAC      (MAPBLOCKSHIFT-FRACBITS)

/* player radius for movement checking */
#define	PLAYERRADIUS	16*FRACUNIT


// MAXRADIUS is for precalculated sector block boxes
// the spider demon is larger,
// but we do not have any moving sectors nearby
#define MAXRADIUS       32*FRACUNIT

#define	GRAVITY			(FRACUNIT*4)    //like JagDoom
#define	MAXMOVE			(16*FRACUNIT)
#define STOPSPEED		0x1000
#define FRICTION		0xd200

#define	USERANGE		(70*FRACUNIT)
#define MELEERANGE      (80*FRACUNIT)       // [d64] changed from 64 to 80
#define ATTACKRANGE     (16*64*FRACUNIT)
#define MISSILERANGE    (32*64*FRACUNIT)
#define LASERRANGE      (4096*FRACUNIT)
#define LASERAIMHEIGHT  (40*FRACUNIT)
#define LASERDISTANCE   (30*FRACUNIT)

// follow a player
#define BASETHRESHOLD   90

//
// P_TICK
//

// both the head and tail of the thinker list
extern    thinker_t    thinkercap;
extern    mobj_t        mobjhead;

void P_InitThinkers(void);
void P_AddThinker(thinker_t* thinker);
void P_RemoveThinker(thinker_t* thinker);
void P_LinkMobj(mobj_t* mobj);
void P_UnlinkMobj(mobj_t* mobj);

extern angle_t frame_angle;
extern angle_t frame_pitch;
extern fixed_t frame_viewx;
extern fixed_t frame_viewy;
extern fixed_t frame_viewz;

//
// P_PSPR
//
typedef struct laser_s {
	fixed_t         x1;
	fixed_t         y1;
	fixed_t         z1;
	fixed_t         x2;
	fixed_t         y2;
	fixed_t         z2;
	fixed_t         slopex;
	fixed_t         slopey;
	fixed_t         slopez;
	fixed_t         dist;
	fixed_t         distmax;
	mobj_t* marker;
	struct laser_s* next;
	angle_t         angle;
} laser_t;

typedef struct {
	thinker_t   thinker;
	mobj_t* dest;
	laser_t* laser;
} laserthinker_t;

void P_SetupPsprites(player_t* curplayer);
void P_MovePsprites(player_t* curplayer);
void P_DropWeapon(player_t* player);
void T_LaserThinker(laserthinker_t* laser);
void P_SetPsprite(player_t* player, int position, statenum_t stnum);
void P_NewPspriteTick(void);

//
// P_USER
//
void    P_PlayerThink(player_t* player);
void    P_Thrust(player_t* player, angle_t angle, fixed_t move);
angle_t P_AdjustAngle(angle_t angle, angle_t newangle, int threshold);
void    P_SetStaticCamera(player_t* player);
void    P_SetFollowCamera(player_t* player);
void    P_ClearUserCamera(player_t* player);

//
// P_MOBJ
//
#define ONFLOORZ        INT_MIN
#define ONCEILINGZ      INT_MAX

extern mapthing_t* spawnlist;
extern int          numspawnlist;

mobj_t* P_SpawnMobj(fixed_t x, fixed_t y, fixed_t z, mobjtype_t type);
void        P_SafeRemoveMobj(mobj_t* mobj);
void        P_RemoveMobj(mobj_t* th);
void        P_SpawnPlayer(mapthing_t* mthing);
void        P_SetTarget(mobj_t** mop, mobj_t* targ);
boolean    P_SetMobjState(mobj_t* mobj, statenum_t state);
void        P_MobjThinker(mobj_t* mobj);
boolean    P_OnMobjZ(mobj_t* mobj);
void        P_NightmareRespawn(mobj_t* mobj);
void        P_RespawnSpecials(mobj_t* special);
void        P_SpawnPuff(fixed_t x, fixed_t y, fixed_t z);
void        P_SpawnBlood(fixed_t x, fixed_t y, fixed_t z, int damage);
void        P_SpawnPlayerMissile(mobj_t* source, mobjtype_t type);
void        P_FadeMobj(mobj_t* mobj, int amount, int alpha, int flags);
int         EV_SpawnMobjTemplate(line_t* line, boolean silent);
int         EV_FadeOutMobj(line_t* line);
void        P_SpawnDartMissile(int tid, int type, mobj_t* target);
mobj_t* P_SpawnMissile(mobj_t* source, mobj_t* dest, mobjtype_t type,
	fixed_t xoffs, fixed_t yoffs, fixed_t heightoffs, boolean aim);

//
// P_ENEMY
//
void P_NoiseAlert(mobj_t* target, mobj_t* emmiter);

//
// P_MAPUTL
//
typedef struct {
	fixed_t    x;
	fixed_t    y;
	fixed_t    dx;
	fixed_t    dy;
} divline_t;

typedef struct {
	fixed_t    frac;        // along trace line
	boolean    isaline;
	union {
		mobj_t* thing;
		line_t* line;
	}            d;
} intercept_t;

#define MAXINTERCEPTS    128

extern intercept_t    intercepts[MAXINTERCEPTS];
extern intercept_t* intercept_p;

typedef boolean(*traverser_t)(intercept_t* in);

fixed_t P_AproxDistance(fixed_t dx, fixed_t dy);
int     P_PointOnLineSide(fixed_t x, fixed_t y, line_t* line);
int     P_PointOnDivlineSide(fixed_t x, fixed_t y, divline_t* line);
void     P_MakeDivline(line_t* li, divline_t* dl);
fixed_t P_InterceptVector(divline_t* v2, divline_t* v1);
void    P_GetIntersectPoint(fixed_t* s1, fixed_t* s2, fixed_t* x, fixed_t* y);
int     P_BoxOnLineSide(fixed_t* tmbox, line_t* ld);

extern fixed_t        opentop;
extern fixed_t         openbottom;
extern fixed_t        openrange;
extern fixed_t        lowfloor;

void         P_LineOpening(line_t* linedef);
boolean    P_BlockLinesIterator(int x, int y, boolean(*func)(line_t*));
boolean    P_BlockThingsIterator(int x, int y, boolean(*func)(mobj_t*));

#define PT_ADDLINES        1
#define PT_ADDTHINGS    2
#define PT_EARLYOUT        4

extern divline_t    trace;

boolean P_PathTraverse(fixed_t x1, fixed_t y1, fixed_t x2, fixed_t y2, int    flags, boolean(*trav)(intercept_t*));
void    P_UnsetThingPosition(mobj_t* thing);
void    P_SetThingPosition(mobj_t* thing);

//
// P_MAP
//

// If "floatok" true, move would be ok
// if within "tmfloorz - tmceilingz".
extern boolean     floatok;
extern fixed_t      tmfloorz;
extern fixed_t      tmceilingz;
extern line_t* tmhitline;

boolean    P_CheckPosition(mobj_t* thing, fixed_t x, fixed_t y);
boolean    P_TryMove(mobj_t* thing, fixed_t x, fixed_t y);
boolean    P_PlayerMove(mobj_t* thing, fixed_t x, fixed_t y);
boolean    P_TeleportMove(mobj_t* thing, fixed_t x, fixed_t y);
void        P_SlideMove(mobj_t* mo);
boolean    P_CheckSight(mobj_t* t1, mobj_t* t2);
void        P_ScanSights(void);
boolean    P_UseLines(player_t* player, boolean showcontext);
boolean    P_ChangeSector(sector_t* sector, boolean crunch);
mobj_t* P_CheckOnMobj(mobj_t* thing);
void        P_CheckChaseCamPosition(mobj_t* target, mobj_t* camera, fixed_t x, fixed_t y);

#define MAXSPECIALCROSS 128
#define MAXTHINGSPEC 8

extern mobj_t* linetarget;    // who got hit (or NULL)
extern mobj_t* blockthing;
extern fixed_t  aimfrac;
extern line_t* thingspec[MAXTHINGSPEC];
extern int      numthingspec;

fixed_t P_AimLineAttack(mobj_t* t1, angle_t angle, fixed_t zheight, fixed_t distance);
void    P_LineAttack(mobj_t* t1, angle_t angle, fixed_t distance, fixed_t slope, int damage);
void    P_RadiusAttack(mobj_t* spot, mobj_t* source, int damage);

//
// P_SETUP
//
extern unsigned char* rejectmatrix;    // for fast sight rejection
extern short* blockmaplump;    // offsets in blockmap are from here
extern short* blockmap;
extern int            bmapwidth;
extern int            bmapheight;    // in mapblocks
extern fixed_t        bmaporgx;
extern fixed_t        bmaporgy;    // origin of block map
extern mobj_t** blocklinks;    // for thing chains

//
// P_INTER
//
extern int        maxammo[NUMAMMO];
extern int        clipammo[NUMAMMO];

extern int infraredFactor;

void P_TouchSpecialThing(mobj_t* special, mobj_t* toucher);
void P_DamageMobj(mobj_t* target, mobj_t* inflictor, mobj_t* source, int damage);

//
// P_SPEC
//

//
// End-level timer (-TIMER option)
//
extern    boolean levelTimer;
extern    int    levelTimeCount;

extern short globalint;

typedef struct {
	int         delay;
	char        name[9];
	int         frames;
	int         speed;
	boolean    reverse;
	boolean    palette;
} animdef_t;

extern int          numanimdef;
extern animdef_t* animdefs;

void        P_InitPicAnims(void);       // at game start
void        P_SpawnSpecials(void);      // at map load
void        P_UpdateSpecials(void);     // every tic
int         P_DoSpecialLine(mobj_t* thing, line_t* line, int side);
void        P_AddSectorSpecial(sector_t* sector);
void        P_SpawnDelayTimer(line_t* line, void (*func)(void));

// when needed
boolean    P_UseSpecialLine(mobj_t* thing, line_t* line, int side);
void        P_PlayerInSpecialSector(player_t* player);

int         twoSided(int sector, int line);
sector_t* getSector(int currentSector, int line, int side);
side_t* getSide(int currentSector, int line, int side);
sector_t* getNextSector(line_t* line, sector_t* sec);

fixed_t     P_FindLowestFloorSurrounding(sector_t* sec);
fixed_t     P_FindHighestFloorSurrounding(sector_t* sec);
fixed_t     P_FindNextHighestFloor(sector_t* sec, int currentheight);
fixed_t     P_FindLowestCeilingSurrounding(sector_t* sec);
fixed_t     P_FindHighestCeilingSurrounding(sector_t* sec);
int         P_FindSectorFromLineTag(line_t* line, int start);
boolean    P_ActivateLineByTag(int tag, mobj_t* activator);

//
// SPECIAL
//

int EV_DoFloorAndCeiling(line_t* line, boolean fast, boolean elevatorOrSplit);

//
// P_LIGHTS
//

typedef struct {
	thinker_t    thinker;
	sector_t* sector;
	int          count;
	int          special;
} fireflicker_t;

typedef struct {
	thinker_t    thinker;
	sector_t* sector;
	int          count;
	int          special;
} lightflash_t;

typedef struct {
	thinker_t    thinker;
	sector_t* sector;
	int          count;
	int          maxlight;
	int          darktime;
	int          brighttime;
	int          special;
} strobe_t;

typedef enum
{
	PULSENORMAL,
	PULSESLOW,
	PULSERANDOM
} glowtype_e;

typedef struct {
	thinker_t    thinker;
	sector_t* sector;
	int          type;
	int          count;
	int          minlight;
	int          direction;
	int          maxlight;
	int          special;
} glow_t;

typedef struct {
	thinker_t    thinker;
	sector_t* sector;
	sector_t* headsector;
	int          count;
	int          start;
	int          index;
	int          special;
} sequenceGlow_t;

typedef struct {
	thinker_t    thinker;
	sector_t* sector;
	thinker_t* combiner;
	int          special;
	actionf_p1   func;
} combine_t;

typedef struct {
	thinker_t thinker;
	light_t* dest;
	light_t* src;
	int r;
	int g;
	int b;
	int inc;
} lightmorph_t;

#define GLOWSPEED			2
#define	STROBEBRIGHT		3
#define	STROBEBRIGHT2		1
#define	TURBODARK			4
#define	FASTDARK			15
#define	SLOWDARK			30

void        P_SpawnFireFlicker(sector_t* sector);
void        T_LightFlash(lightflash_t* flash);
void        P_SpawnLightFlash(sector_t* sector);
void        T_StrobeFlash(strobe_t* flash);
void        T_FireFlicker(fireflicker_t* flick);
void        P_UpdateLightThinker(light_t* destlight, light_t* srclight);
void        T_Sequence(sequenceGlow_t* seq);
void        P_SpawnStrobeFlash(sector_t* sector, int speed);
void        P_SpawnStrobeAltFlash(sector_t* sector, int speed);
void        EV_StartLightStrobing(line_t* line);
void        T_Glow(glow_t* g);
void        P_SpawnGlowingLight(sector_t* sector, unsigned char type);
void        P_SpawnSequenceLight(sector_t* sector, boolean first);
void        P_CombineLightSpecials(sector_t* sector);
void        T_Combine(combine_t* combine);
boolean    P_ChangeLightByTag(int tag1, int tag2);
int         P_DoSectorLightChange(line_t* line, short tag);
void        P_FadeInBrightness(void);
boolean		P_StartSound(int index);
boolean		P_ChangeMusic(int index);
boolean		P_ChangeSky(int index);
boolean		P_SpawnGenericMissile(int tid, int type, mobj_t* target);

typedef enum
{
	top,
	middle,
	bottom
} bwhere_e;

typedef struct
{
	side_t* side;  //old line_t		*line;
	bwhere_e	where;
	int			btexture;
	int			btimer;
	mobj_t* soundorg;
} button_t;

// max # of wall switches in a level
#define MAXSWITCHES       50

// 4 players, 4 buttons each at once, max.
#define MAXBUTTONS        16

// 1 second, in ticks.
#define BUTTONTIME      15

extern button_t    buttonlist[MAXBUTTONS];

void P_ChangeSwitchTexture(line_t* line, int useAgain);

//
// P_PLATS
//
typedef enum {
	up,
	down,
	waiting,
	in_stasis
} plat_e;

typedef enum {
	perpetualRaise,
	downWaitUpStay,
	raiseAndChange,
	raiseToNearestAndChange,
	blazeDWUS,
	upWaitDownStay,
	blazeUWDS,
	customDownUp,
	customDownUpFast,
	customUpDown,
	customUpDownFast
} plattype_e;

typedef struct {
	thinker_t    thinker;
	sector_t* sector;
	fixed_t      speed;
	fixed_t      low;
	fixed_t      high;
	int          wait;
	int          count;
	plat_e       status;
	plat_e       oldstatus;
	boolean     crush;
	int          tag;
	plattype_e   type;
} plat_t;

#define	PLATWAIT	3			/* seconds */
#define	PLATSPEED	(FRACUNIT*2)
#define MAXPLATS	60

extern plat_t* activeplats[MAXPLATS];

void    T_PlatRaise(plat_t* plat);
int        EV_DoPlat(line_t* line, plattype_e type, int amount);
void    P_AddActivePlat(plat_t* plat);
void    P_RemoveActivePlat(plat_t* plat);
void    EV_StopPlat(line_t* line);
void    P_ActivateInStasis(int tag);

//
// P_DOORS
//

typedef enum {
	normal,
	close30ThenOpen,
	doorclose,
	dooropen,
	raiseIn5Mins,
	blazeRaise,
	blazeOpen,
	blazeClose
} vldoor_e;

typedef struct {
	thinker_t   thinker;
	vldoor_e    type;
	sector_t* sector;
	fixed_t     topheight;
	fixed_t     bottomheight;
	fixed_t     initceiling;
	fixed_t     speed;

	// 1 = up, 0 = waiting at top, -1 = down
	int         direction;

	// tics to wait at the top
	int         topwait;

	// (keep in case a door going down is reset)
	// when it reaches 0, start going down
	int         topcountdown;
} vldoor_t;

#define	VDOORSPEED	FRACUNIT*2
#define	VDOORWAIT	120

void    EV_VerticalDoor(line_t* line, mobj_t* thing);
int     EV_DoDoor(line_t* line, vldoor_e type);
void    T_VerticalDoor(vldoor_t* door);

//
// P_CEILNG
//
typedef enum {
	lowerToFloor,
	raiseToHighest,
	lowerAndCrush,
	crushAndRaise,
	fastCrushAndRaise,
	silentCrushAndRaise,
	customCeiling,
	crushAndRaiseOnce,
	customCeilingToHeight
} ceiling_e;

typedef struct {
	thinker_t    thinker;
	ceiling_e    type;
	sector_t* sector;
	fixed_t      bottomheight;
	fixed_t      topheight;
	fixed_t      speed;
	boolean     crush;
	int          direction;    // 1 = up, 0 = waiting, -1 = down
	int          tag;        // ID
	int          olddirection;
	boolean     instant;
} ceiling_t;

#define CEILSPEED       FRACUNIT*2
#define CEILWAIT        150
#define MAXCEILINGS     30

extern ceiling_t* activeceilings[MAXCEILINGS];

int        EV_DoCeiling(line_t* line, ceiling_e type, fixed_t speed);
void    T_MoveCeiling(ceiling_t* ceiling);
void    P_AddActiveCeiling(ceiling_t* c);
void    P_RemoveActiveCeiling(ceiling_t* c);
int        EV_CeilingCrushStop(line_t* line);
void    P_ActivateInStasisCeiling(line_t* line);

//
// P_FLOOR
//
typedef enum {
	lowerFloor,             // lower floor to highest surrounding floor
	lowerFloorToLowest,     // lower floor to lowest surrounding floor
	turboLower,             // lower floor to highest surrounding floor VERY FAST
	raiseFloor,             // raise floor to lowest surrounding CEILING
	raiseFloorToNearest,    // raise floor to next highest surrounding floor
	lowerAndChange,         // lower floor to lowest surrounding floor and change floorpic
	raiseFloor24,
	raiseFloor24AndChange,
	raiseFloorCrush,
	customFloor,
	customFloorToHeight
} floor_e;

typedef enum {
	build8, // slowly build by 8
	turbo16 // quickly build by 16
} stair_e;

typedef struct {
	thinker_t    thinker;
	floor_e      type;
	boolean     crush;
	sector_t* sector;
	int          direction;
	int          newspecial;
	short        texture;
	fixed_t      floordestheight;
	fixed_t      speed;
	boolean     instant;
} floormove_t;

typedef struct {
	thinker_t    thinker;
	sector_t* sector;
	fixed_t      ceildest;
	fixed_t      flrdest;
	int          ceildir;
	int          flrdir;
} splitmove_t;

#define FLOORSPEED FRACUNIT * 3

typedef enum {
	ok,
	crushed,
	pastdest,
	stop
} result_e;

result_e T_MovePlane(sector_t* sector, fixed_t speed, fixed_t dest,
	boolean crush, int floorOrCeiling, int direction);

int EV_BuildStairs(line_t* line, stair_e type);
int EV_DoFloor(line_t* line, floor_e floortype, fixed_t speed);
void T_MoveFloor(floormove_t* floor);
void T_MoveSplitPlane(splitmove_t* split);
int EV_SplitSector(line_t* line, boolean sync);

//
// P_TELEPT
//
int EV_Teleport(line_t* line, int side, mobj_t* thing);
int EV_SilentTeleport(line_t* line, mobj_t* thing);

// Misc stuff

typedef struct {
	thinker_t thinker;
	int tics;
	void (*finishfunc)(void);
} delay_t;

typedef struct {
	thinker_t thinker;
	mobj_t* viewmobj;
	player_t* activator;
} aimcamera_t;

typedef struct {
	thinker_t thinker;
	fixed_t x;
	fixed_t y;
	fixed_t z;
	fixed_t slopex;
	fixed_t slopey;
	fixed_t slopez;
	player_t* player;
	int current;
	int tic;
} movecamera_t;

typedef struct {
	thinker_t thinker;
	mobj_t* mobj;
	int amount;
	int destAlpha;
	int flagReserve;
} mobjfade_t;

typedef struct {
	thinker_t thinker;
	int delay;
	int lifetime;
	int delaymax;
	mobj_t* mobj;
} mobjexp_t;

typedef struct {
	thinker_t thinker;
	int tics;
} quake_t;

typedef struct {
	thinker_t   thinker;
	fixed_t     x1;
	fixed_t     x2;
	fixed_t     y1;
	fixed_t     y2;
	fixed_t     z;
	int         flags;
	int         tic;
} tracedrawer_t;

//thinkers

void T_LightMorph(lightmorph_t* lt);
void T_CountdownTimer(delay_t* timer);
void T_MobjExplode(mobjexp_t* mexp);
void T_LookAtCamera(aimcamera_t* camera);
void T_MovingCamera(movecamera_t* camera);
void T_MobjFadeThinker(mobjfade_t* mobjfade);
void T_Quake(quake_t* quake);
void T_TraceDrawer(tracedrawer_t* tdrawer);

#endif    // __P_LOCAL__
