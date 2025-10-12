// Emacs style mode select   -*- C -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 2005 Simon Howard
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
// Handles merging of PWADs, similar to deutex's -merge option
//
// Ideally this should work exactly the same as in deutex, but trying to
// read the deutex source code made my brain hurt.
//
//-----------------------------------------------------------------------------

#include <stdlib.h>
#include <string.h>
#include <SDL3/SDL_stdinc.h>

#include "w_merge.h"
#include "doomdef.h"
#include "i_system.h"
#include "w_wad.h"
#include "z_zone.h"

typedef enum {
	SECTION_NORMAL,
	SECTION_TEXTURES,
	SECTION_SPRITES,
	SECTION_GFX,
	SECTION_SOUNDS
} section_t;

typedef struct {
	lumpinfo_t* lumps;
	int numlumps;
} searchlist_t;

typedef struct {
	char sprname[4];
	char frame;
	lumpinfo_t* angle_lumps[8];
} sprite_frame_t;

static searchlist_t iwad;
static searchlist_t iwad_sprites;
static searchlist_t iwad_textures;
static searchlist_t iwad_gfx;
static searchlist_t iwad_sounds;

static searchlist_t pwad;
static searchlist_t pwad_sprites;
static searchlist_t pwad_textures;
static searchlist_t pwad_gfx;
static searchlist_t pwad_dm_sounds;
static searchlist_t pwad_ds_sounds;

// lumps with these sprites must be replaced in the IWAD
static sprite_frame_t* sprite_frames;
static int num_sprite_frames;
static int sprite_frames_alloced;

// Search in a list to find a lump with a particular name
// Linear search (slow!)
//
// Returns -1 if not found

static int FindInList(searchlist_t* list, const char* name) {
	int i;

	for (i = 0; i < list->numlumps; ++i) {
		if (!dstrnicmp(list->lumps[i].name, name, 8)) {
			return i;
		}
	}

	return -1;
}

static boolean SetupList(searchlist_t* list, searchlist_t* src_list,
	const char* startname, const char* endname,
	const char* startname2, const char* endname2) {
	int startlump, endlump;

	list->numlumps = 0;
	startlump = FindInList(src_list, startname);

	if (startname2 != NULL && startlump < 0) {
		startlump = FindInList(src_list, startname2);
	}

	if (startlump >= 0) {
		endlump = FindInList(src_list, endname);

		if (endname2 != NULL && endlump < 0) {
			endlump = FindInList(src_list, endname2);
		}

		if (endlump > startlump) {
			list->lumps = src_list->lumps + startlump + 1;
			list->numlumps = endlump - startlump - 1;
			return true;
		}
	}

	return false;
}

// Sets up the sprite/flat search lists
static void SetupLists(void)
{
    // IWAD
    if (!SetupList(&iwad_textures, &iwad, "T_START", "T_END", NULL, NULL)) {
        I_Error("Textures section not found in IWAD");
    }
    if (!SetupList(&iwad_sprites, &iwad, "S_START", "S_END", NULL, NULL)) {
        I_Error("Sprites section not found in IWAD");
    }
    if (!SetupList(&iwad_gfx, &iwad, "SYMBOLS", "MOUNTC", NULL, NULL)) {
        I_Error("GFX section not found in IWAD");
    }
    if (!SetupList(&iwad_sounds, &iwad, "DM_START", "DM_END", "DS_START", "DS_END")) {
        I_Error("Sounds section not found in IWAD");
    }

    // PWAD
    SetupList(&pwad_textures, &pwad, "T_START", "T_END", "TT_START", "TT_END");
    SetupList(&pwad_sprites, &pwad, "S_START", "S_END", "SS_START", "SS_END");
    SetupList(&pwad_gfx, &pwad, "G_START", "G_END", "SYMBOLS", "MOUNTC");
    SetupList(&pwad_dm_sounds, &pwad, "DM_START", "DM_END", NULL, NULL);
    SetupList(&pwad_ds_sounds, &pwad, "DS_START", "DS_END", NULL, NULL);
}

// Initialise the replace list

static void InitSpriteList(void) {
	if (sprite_frames == NULL) {
		sprite_frames_alloced = 128;
		sprite_frames = (sprite_frame_t*)Z_Malloc(sizeof(*sprite_frames) * sprite_frames_alloced,
			PU_STATIC, NULL);
	}

	num_sprite_frames = 0;
}

// Find a sprite frame

#ifdef GCC_COMPILER
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-truncation"
#endif

static sprite_frame_t* FindSpriteFrame(char* name, int frame) {
	sprite_frame_t* result;
	int i;

	// Search the list and try to find the frame

	for (i = 0; i < num_sprite_frames; ++i) {
		sprite_frame_t* cur = &sprite_frames[i];

		if (!dstrnicmp(cur->sprname, name, 4) && cur->frame == frame) {
			return cur;
		}
	}

	// Not found in list; Need to add to the list

	// Grow list?

	if (num_sprite_frames >= sprite_frames_alloced) {
		sprite_frame_t* newframes;

		newframes = (sprite_frame_t*)Z_Malloc(sprite_frames_alloced * 2 * sizeof(*sprite_frames),
			PU_STATIC, NULL);
		dmemcpy(newframes, sprite_frames,
			sprite_frames_alloced * sizeof(*sprite_frames));
		Z_Free(sprite_frames);
		sprite_frames_alloced *= 2;
		sprite_frames = newframes;
	}

	// Add to end of list

	result = &sprite_frames[num_sprite_frames];
	strncpy(result->sprname, name, 4);
	result->frame = frame;

	for (i = 0; i < 8; ++i) {
		result->angle_lumps[i] = NULL;
	}

	++num_sprite_frames;

	return result;
}

#ifdef GCC_COMPILER
#pragma GCC diagnostic pop
#endif


// Check if sprite lump is needed in the new wad

static boolean SpriteLumpNeeded(lumpinfo_t* lump) {
	sprite_frame_t* sprite;
	int angle_num;
	int i;

	// check the first frame

	sprite = FindSpriteFrame(lump->name, lump->name[4]);
	angle_num = lump->name[5] - '0';

	if (angle_num == 0) {
		// must check all frames

		for (i = 0; i < 8; ++i) {
			if (sprite->angle_lumps[i] == lump) {
				return true;
			}
		}
	}
	else {
		// check if this lump is being used for this frame

		if (sprite->angle_lumps[angle_num - 1] == lump) {
			return true;
		}
	}

	// second frame if any

	// no second frame?
	if (lump->name[6] == '\0') {
		return false;
	}

	sprite = FindSpriteFrame(lump->name, lump->name[6]);
	angle_num = lump->name[7] - '0';

	if (angle_num == 0) {
		// must check all frames

		for (i = 0; i < 8; ++i) {
			if (sprite->angle_lumps[i] == lump) {
				return true;
			}
		}
	}
	else {
		// check if this lump is being used for this frame

		if (sprite->angle_lumps[angle_num - 1] == lump) {
			return true;
		}
	}

	return false;
}

static void AddSpriteLump(lumpinfo_t* lump) {
	sprite_frame_t* sprite;
	int angle_num;
	int i;

	sprite = FindSpriteFrame(lump->name, lump->name[4]);
	angle_num = lump->name[5] - '0';

	if (angle_num == 0) {
		for (i = 0; i < 8; ++i) {
			sprite->angle_lumps[i] = lump;
		}
	}
	else {
		sprite->angle_lumps[angle_num - 1] = lump;
	}

	if (lump->name[6] == '\0') {
		return;
	}

	sprite = FindSpriteFrame(lump->name, lump->name[6]);
	angle_num = lump->name[7] - '0';

	if (angle_num == 0) {
		for (i = 0; i < 8; ++i) {
			sprite->angle_lumps[i] = lump;
		}
	}
	else {
		sprite->angle_lumps[angle_num - 1] = lump;
	}
}

static void GenerateSpriteList(void) {
	int i;

	InitSpriteList();

	for (i = 0; i < iwad_sprites.numlumps; ++i) {
		AddSpriteLump(&iwad_sprites.lumps[i]);
	}

	for (i = 0; i < pwad_sprites.numlumps; ++i) {
		AddSpriteLump(&pwad_sprites.lumps[i]);
	}
}

static int LumpLooksGraphic(const lumpinfo_t* li) {
	return (li && li->size >= 8);
}

static int FindInPWAD(const char* name) {
	int i;
	for (i = 0; i < pwad.numlumps; ++i) {
		if (!dstrnicmp(pwad.lumps[i].name, name, 8))
			return i;
	}
	return -1;
}

// Perform the merge.
static void DoMerge(void)
{
    section_t    current_section;
    lumpinfo_t* newlumps;
    int          num_newlumps;
    int          lumpindex;
    int          i, n;

    newlumps = (lumpinfo_t*)malloc(sizeof(lumpinfo_t) * numlumps);
    num_newlumps = 0;


    // IWAD
    current_section = SECTION_NORMAL;

    for (i = 0; i < iwad.numlumps; ++i) {
        lumpinfo_t* lump = &iwad.lumps[i];

        if (lump->name[0] == 'M' && lump->name[1] == 'A' && lump->name[2] == 'P' &&
            lump->name[3] >= '0' && lump->name[3] <= '9' &&
            lump->name[4] >= '0' && lump->name[4] <= '9')
        {
            int pw = FindInPWAD(lump->name);
            if (pw >= 0) {
                int k = i + 1;
                for (;;) {
                    if (k >= iwad.numlumps) break;
                    int is_sub = 0;
                    char* subs[] = {
                        "THINGS","LINEDEFS","SIDEDEFS","VERTEXES","SEGS","SSECTORS",
                        "NODES","SECTORS","REJECT","BLOCKMAP","BEHAVIOR","LEAFS","LIGHTS","MACROS"
                    };
                    for (int si = 0; si < SDL_arraysize(subs); ++si) {
                        if (W_LumpNameEq(&iwad.lumps[k], subs[si])) { is_sub = 1; break; }
                    }
                    if (!is_sub) break;
                    ++k;
                }
                i = k - 1;
                continue;
            }
        }

        switch (current_section) {
        case SECTION_NORMAL:
            if (W_LumpNameEq(lump, "T_START")) {
                current_section = SECTION_TEXTURES;
                newlumps[num_newlumps++] = *lump;
            }
            else if (W_LumpNameEq(lump, "S_START")) {
                current_section = SECTION_SPRITES;
                newlumps[num_newlumps++] = *lump;
            }
            else if (W_LumpNameEq(lump, "SYMBOLS")) {
                current_section = SECTION_GFX;

                const int pw_sym = FindInPWAD("SYMBOLS");
                if (pw_sym >= 0 && LumpLooksGraphic(&pwad.lumps[pw_sym])) {
                    newlumps[num_newlumps++] = pwad.lumps[pw_sym];
                }
                else if (LumpLooksGraphic(lump)) {
                    newlumps[num_newlumps++] = *lump;
                }
            }
            else if (W_LumpNameEq(lump, "DM_START") || W_LumpNameEq(lump, "DS_START")) {
                current_section = SECTION_SOUNDS;
                newlumps[num_newlumps++] = *lump;
            }
            else {
                newlumps[num_newlumps++] = *lump;
            }
            break;

        case SECTION_TEXTURES:
            if (W_LumpNameEq(lump, "T_END")) {
                for (n = 0; n < pwad_textures.numlumps; ++n)
                    newlumps[num_newlumps++] = pwad_textures.lumps[n];

                newlumps[num_newlumps++] = *lump;
                current_section = SECTION_NORMAL;
            }
            else {
                lumpindex = FindInList(&pwad_textures, lump->name);
                if (lumpindex < 0)
                    newlumps[num_newlumps++] = *lump;
            }
            break;

        case SECTION_SPRITES:
            if (W_LumpNameEq(lump, "S_END")) {
                for (n = 0; n < pwad_sprites.numlumps; ++n)
                    if (SpriteLumpNeeded(&pwad_sprites.lumps[n]))
                        newlumps[num_newlumps++] = pwad_sprites.lumps[n];

                newlumps[num_newlumps++] = *lump;
                current_section = SECTION_NORMAL;
            }
            else {
                if (SpriteLumpNeeded(lump))
                    newlumps[num_newlumps++] = *lump;
            }
            break;

        case SECTION_GFX:
            if (W_LumpNameEq(lump, "MOUNTC")) {
                for (n = 0; n < pwad_gfx.numlumps; ++n)
                    newlumps[num_newlumps++] = pwad_gfx.lumps[n];

                const int pw_tail = FindInPWAD("MOUNTC");
                if (pw_tail >= 0 && LumpLooksGraphic(&pwad.lumps[pw_tail])) {
                    newlumps[num_newlumps++] = pwad.lumps[pw_tail];
                }
                else if (LumpLooksGraphic(lump)) {
                    newlumps[num_newlumps++] = *lump;
                }
                current_section = SECTION_NORMAL;
            }
            else {
                lumpindex = FindInList(&pwad_gfx, lump->name);
                if (lumpindex < 0)
                    newlumps[num_newlumps++] = *lump;
            }
            break;

        case SECTION_SOUNDS:
            if (W_LumpNameEq(lump, "DM_END")) {
                for (n = 0; n < pwad_dm_sounds.numlumps; ++n)
                    newlumps[num_newlumps++] = pwad_dm_sounds.lumps[n];

                newlumps[num_newlumps++] = *lump;
                current_section = SECTION_NORMAL;
            }
            else if (W_LumpNameEq(lump, "DS_END")) {
                for (n = 0; n < pwad_ds_sounds.numlumps; ++n)
                    newlumps[num_newlumps++] = pwad_ds_sounds.lumps[n];

                newlumps[num_newlumps++] = *lump;
                current_section = SECTION_NORMAL;
            }
            else {
                lumpindex = FindInList(&pwad_dm_sounds, lump->name);
                if (lumpindex < 0) {
                    lumpindex = FindInList(&pwad_ds_sounds, lump->name);
                }
                if (lumpindex < 0) {
                    newlumps[num_newlumps++] = *lump;
                }
            }
            break;
        }
    }

    // PWAD
    current_section = SECTION_NORMAL;

    for (i = 0; i < pwad.numlumps; ++i) {
        lumpinfo_t* lump = &pwad.lumps[i];

        if (lump->name[0] == 'M' && lump->name[1] == 'A' && lump->name[2] == 'P' &&
            lump->name[3] >= '0' && lump->name[3] <= '9' &&
            lump->name[4] >= '0' && lump->name[4] <= '9')
        {
            newlumps[num_newlumps++] = *lump; // marker

            int j = i + 1;
            for (;;) {
                if (j >= pwad.numlumps) break;
                int is_sub = 0;
                char* subs[] = {
                    "THINGS","LINEDEFS","SIDEDEFS","VERTEXES","SEGS","SSECTORS",
                    "NODES","SECTORS","REJECT","BLOCKMAP","BEHAVIOR","LEAFS","LIGHTS","MACROS"
                };
                for (int si = 0; si < (int)(sizeof(subs) / sizeof(subs[0])); ++si) {
                    if (W_LumpNameEq(&pwad.lumps[j], subs[si])) { is_sub = 1; break; }
                }
                if (!is_sub) break;
                newlumps[num_newlumps++] = pwad.lumps[j++];
            }
            i = j - 1;
            continue;
        }

        switch (current_section) {
        case SECTION_NORMAL:
            if (W_LumpNameEq(lump, "T_START") || W_LumpNameEq(lump, "TT_START")) {
                current_section = SECTION_TEXTURES;
            }
            else if (W_LumpNameEq(lump, "S_START") || W_LumpNameEq(lump, "SS_START")) {
                current_section = SECTION_SPRITES;
            }
            else if (W_LumpNameEq(lump, "SYMBOLS") || W_LumpNameEq(lump, "G_START")) {
                current_section = SECTION_GFX;
            }
            else if (W_LumpNameEq(lump, "DM_START") || W_LumpNameEq(lump, "DS_START")) {
                current_section = SECTION_SOUNDS;
            }
            else {
                newlumps[num_newlumps++] = *lump;
            }
            break;

        case SECTION_TEXTURES:
            if (W_LumpNameEq(lump, "TT_END") || W_LumpNameEq(lump, "T_END")) {
                current_section = SECTION_NORMAL;
            }
            break;

        case SECTION_SPRITES:
            if (W_LumpNameEq(lump, "SS_END") || W_LumpNameEq(lump, "S_END")) {
                current_section = SECTION_NORMAL;
            }
            break;

        case SECTION_GFX:
            if (W_LumpNameEq(lump, "MOUNTC") || W_LumpNameEq(lump, "G_END")) {
                current_section = SECTION_NORMAL;
            }
            break;

        case SECTION_SOUNDS:
            if (W_LumpNameEq(lump, "DM_END") || W_LumpNameEq(lump, "DS_END")) {
                current_section = SECTION_NORMAL;
            }
            break;
        }
    }

    free(lumpinfo);
    lumpinfo = newlumps;
    numlumps = num_newlumps;
}

void W_PrintDirectory(void) {
	int i, n;

	// debug
	for (i = 0; i < numlumps; ++i) {
		for (n = 0; n < 8 && lumpinfo[i].name[n] != '\0'; ++n) {
			putchar(lumpinfo[i].name[n]);
		}

		putchar('\n');
	}
}

// Merge in a file by name
void W_MergeFile(char* filename) {
	int old_numlumps;

	old_numlumps = numlumps;

	// Load PWAD

	if (!W_AddFile(filename)) return;

	// iwad is at the start, pwad was appended to the end

	iwad.lumps = lumpinfo;
	iwad.numlumps = old_numlumps;

	pwad.lumps = lumpinfo + old_numlumps;
	pwad.numlumps = numlumps - old_numlumps;

	// Setup sprite/flat lists

	SetupLists();

	// Generate list of sprites to be replaced by the PWAD

	GenerateSpriteList();

	// Perform the merge

	DoMerge();
}
