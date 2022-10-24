// Emacs style mode select   -*- C++ -*-
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
//
//
// DESCRIPTION:
//        Game completion, final screen animation.
//
//-----------------------------------------------------------------------------

#include <ctype.h>
#include "i_system.h"
#include "z_zone.h"
#include "w_wad.h"
#include "s_sound.h"
#include "d_englsh.h"
#include "sounds.h"
#include "doomstat.h"
#include "t_bsp.h"
#include "g_local.h"
#include "info.h"
#include "r_local.h"
#include "st_stuff.h"
#include "r_wipe.h"
#include "gl_draw.h"

static int          castrotation = 0;
static int          castnum;
static int          casttics;
static state_t      *caststate;
static dboolean     castdeath;
static dboolean     castdying;
static int          castframes;
static int          castonmelee;
static dboolean     castattacking;
static dPalette_t   finalePal;

typedef struct {
    char *name;
    mobjtype_t type;
} castinfo_t;

castinfo_t castorder[] = {
    {   CC_ZOMBIE,  MT_POSSESSED1   },
    {   CC_SHOTGUN, MT_POSSESSED2   },
    {   CC_IMP,     MT_IMP1         },
    {   CC_IMP2,    MT_IMP2         },
    {   CC_DEMON,   MT_DEMON1       },
    {   CC_DEMON2,  MT_DEMON2       },
    {   CC_LOST,    MT_SKULL        },
    {   CC_CACO,    MT_CACODEMON    },
    {   CC_HELL,    MT_BRUISER2     },
    {   CC_BARON,   MT_BRUISER1     },
    {   CC_ARACH,   MT_BABY         },
    {   CC_PAIN,    MT_PAIN         },
    {   CC_MANCU,   MT_MANCUBUS     },
    {   CC_CYBER,   MT_CYBORG       },
    {   CC_HERO,    MT_PLAYER       },
    {   NULL,       0               }
};

//
// F_Start
//

void F_Start(void) {
    gameaction = ga_nothing;
    automapactive = false;

    castnum = 0;
    caststate = &states[mobjinfo[castorder[castnum].type].seestate];
    casttics = caststate->tics;
    castdeath = false;
    castframes = 0;
    castonmelee = 0;
    castattacking = false;
    castdying = false;

    finalePal.a = 255;
    finalePal.r = 0;
    finalePal.g = 0;
    finalePal.b = 0;

    // hack - force-play seesound from first cast
    S_StartSound(NULL, mobjinfo[castorder[castnum].type].seesound);
}

//
// F_Stop
//

void F_Stop(void) {
    S_StopMusic();
    //gameaction = ga_nothing;
    WIPE_FadeScreen(6);
}

//
// F_Ticker
//

int F_Ticker(void) {
    int st;
    playercontrols_t* pc = &Controls;

    if(!castdeath) {
        if(pc->key[PCKEY_LEFT]) {
            castrotation = castrotation+1 & 7;
            pc->key[PCKEY_LEFT] = 0;
        }
        else if(pc->key[PCKEY_RIGHT]) {
            castrotation = castrotation-1 & 7;
            pc->key[PCKEY_RIGHT] = 0;
        }
        else if(players[consoleplayer].cmd.buttons) {
            castdying = true;
        }
    }

    finalePal.r = MIN(finalePal.r += 2, 250);
    finalePal.g = MIN(finalePal.g += 2, 250);
    finalePal.b = MIN(finalePal.b += 2, 250);

    if(!castdeath && castdying) {
        S_StartSound(NULL, sfx_shotgun);
        S_StartSound(NULL, mobjinfo[castorder[castnum].type].deathsound);
        caststate = &states[mobjinfo[castorder[castnum].type].deathstate];
        casttics = caststate->tics;
        castframes = 0;
        castattacking = false;
        castdying = false;
        castdeath = true;
    }

    // advance state
    if(--casttics > 0) {
        return 0;    // not time to change state yet
    }

    if(caststate->tics == -1 || caststate->nextstate == S_NULL) {
        // switch from deathstate to next monster

        castnum++;
        castdeath = false;
        castrotation = 0;
        if(castorder[castnum].name == NULL) {
            castnum = 0;
        }

        finalePal.a = 255;
        finalePal.r = 0;
        finalePal.g = 0;
        finalePal.b = 0;

        if(mobjinfo[castorder[castnum].type].seesound) {
            S_StartSound(NULL, mobjinfo[castorder[castnum].type].seesound);
        }

        caststate = &states[mobjinfo[castorder[castnum].type].seestate];
        castframes = 0;
    }
    else {
        // just advance to next state in animation

        if(caststate == &states[S_PLAY_ATK2]) { // gross hack..
            goto stopattack;
        }

        st = caststate->nextstate;
        caststate = &states[st];
        castframes++;

        // sound hacks
        {
            int sound = 0;

            switch(st) {
            case S_PLAY_ATK2:                           // player
                sound = sfx_sht2fire;
                break;
            case S_SARG_ATK2:                           // demon
                sound = sfx_sargatk;
                break;
            case S_FATT_ATK2:                           // mancubus
            case S_FATT_ATK4:                           // mancubus
            case S_FATT_ATK6:                           // mancubus
            case S_TROO_ATK2:                           // imp
            case S_HEAD_ATK3:                           // cacodemon
            case S_BOSS1_ATK3:                          // hell knight
            case S_BOSS2_ATK3:                          // baron
                sound = sfx_bdmissile;
                break;
            case S_TROO_MELEE3:                         // imp scratch
                sound = sfx_scratch;
                break;
            case S_POSS1_ATK2:                          // former human
                sound = sfx_pistol;
                break;
            case S_POSS2_ATK2:                          // shotgun guy
                sound = sfx_shotgun;
                break;
            case S_SKUL_ATK2:                           // skull
            case S_PAIN_ATK4:                           // pain
                sound = sfx_skullatk;
                break;
            case S_BSPI_ATK2:                           // arachnotron
                sound = sfx_plasma;
                break;
            case S_CYBR_ATK2:                           // cyberdemon
            case S_CYBR_ATK4:
            case S_CYBR_ATK6:
                sound = sfx_missile;
                break;
            default:
                sound = 0;
                break;
            }

            if(sound) {
                S_StartSound(NULL, sound);
            }
        }
    }

    if(castframes == 12) {
        // go into attack frame
        castattacking = true;
        if(castonmelee) {
            caststate = &states[mobjinfo[castorder[castnum].type].meleestate];
        }
        else {
            caststate = &states[mobjinfo[castorder[castnum].type].missilestate];
        }
        castonmelee ^= 1;

        if(caststate == &states[S_NULL]) {
            if(castonmelee) {
                caststate = &states[mobjinfo[castorder[castnum].type].meleestate];
            }
            else {
                caststate = &states[mobjinfo[castorder[castnum].type].missilestate];
            }
        }
    }

    if(castattacking) {
        if(castframes == 24 ||
                caststate == &states[mobjinfo[castorder[castnum].type].seestate]) {
stopattack:
            castattacking = false;
            castframes = 0;
            caststate = &states[mobjinfo[castorder[castnum].type].seestate];
        }
    }

    casttics = caststate->tics;
    if(casttics == -1) {
        casttics = TICRATE;
    }

    return 0;
}


//
// F_Drawer
//

void F_Drawer(void) {
    GL_ClearView(0xFF000000);
    Draw_GfxImage(64, 30, "EVIL", D_RGBA(255, 255, 255, 0xff), false);
    Draw_BigText(-1, 240-32, D_RGBA(255, 0, 0, 0xff), castorder[castnum].name);
    Draw_Sprite2D(
        caststate->sprite,
        castrotation,
        (caststate->frame & FF_FRAMEMASK),
        160,
        180,
        1.0f,
        mobjinfo[castorder[castnum].type].palette,
        D_RGBA(finalePal.r, finalePal.g, finalePal.b, finalePal.a)
    );
}


