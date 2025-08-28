// Emacs style mode select   -*- C -*-
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
//    Handles WAD file header, directory, lump I/O.
//
//-----------------------------------------------------------------------------

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>   // for intptr_t
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_timer.h>

#include "w_wad.h"
#include "w_merge.h"
#include "w_file.h"
#include "doomstat.h"
#include "i_system.h"
#include "i_system_io.h"
#include "z_zone.h"
#include "m_misc.h"
#include "md5.h"
#include "i_swap.h"
#include "kpf.h"
#include "i_png.h"

#define KPF_PNG_CAP_BYTES (40 * 1024 * 1024)
#define WADFILE_MEM ((wad_file_t*)(intptr_t)-1)

typedef struct memlump_s {
	unsigned char* data;
	int size;
	char name[8];
} memlump_t;

#ifndef MAX_MEMLUMPS
#define MAX_MEMLUMPS 16
#endif

static memlump_t g_memlumps[MAX_MEMLUMPS];
static int g_nmemlumps = 0;

extern int win_px_w, win_px_h;

#pragma pack(push, 1)

//
// TYPES
//
typedef struct {
	char        identification[4];
	int            numlumps;
	int            infotableofs;
} wadinfo_t;

typedef struct {
	int            filepos;
	int            size;
	char        name[8];
} filelump_t;

#pragma pack(pop)

#define MAX_MEMLUMPS    16

// Location of each lump on disk.
extern lumpinfo_t* lumpinfo;
int            numlumps;

#define CopyLumps(dest, src, count) dmemcpy(dest, src, (count)*sizeof(lumpinfo_t))
#define CopyLump(dest, src) CopyLumps(dest, src, 1)

void ExtractFileBase(char* path, char* dest) {
	char* src;
	int        length;

	src = path + dstrlen(path) - 1;

	// back up until a \ or the start
	while (src != path
		&& *(src - 1) != '\\'
		&& *(src - 1) != '/') {
		src--;
	}

	// copy up to eight characters
	dmemset(dest, 0, 8);
	length = 0;

	while (*src && *src != '.') {
		if (++length == 9) {
			I_Error("Filename base of %s >8 chars", path);
		}

		*dest++ = SDL_toupper((int)*src++);
	}
}

//
// W_HashLumpName
//

unsigned int W_HashLumpName(const char* str) {
	unsigned int hash = 1315423911;
	unsigned int i = 0;

	for (i = 0; i < 8 && *str != '\0'; str++, i++) {
		hash ^= ((hash << 5) + SDL_toupper((int)*str) + (hash >> 2));
	}

	return hash;
}

//
// W_HashLumps
//

static void W_HashLumps(void) {
	int i;
	unsigned int hash;

	for (i = 0; i < numlumps; i++) {
		lumpinfo[i].index = -1;
		lumpinfo[i].next = -1;
	}

	for (i = 0; i < numlumps; i++) {
		hash = (W_HashLumpName(lumpinfo[i].name) % numlumps);
		lumpinfo[i].next = lumpinfo[hash].index;
		lumpinfo[hash].index = i;
	}
}

//
// W_AddFile
// All files are optional, but at least one file must be
//  found (PWAD, if all required lumps are present).
// Files with a .wad extension are wadlink files
//  with multiple lumps.
// Other files are single lumps with the base filename
//  for the lump name.

wad_file_t* W_AddFile(char* filename) {
	wadinfo_t   header;
	lumpinfo_t* lump_p;
	int         i;
	wad_file_t* wadfile;
	int         length;
	int         startlump;
	filelump_t* fileinfo;
	filelump_t* filerover;

	// open the file and add to directory

	wadfile = W_OpenFile(filename);

	if (wadfile == NULL) {
		I_Printf("W_AddFile: couldn't open %s\n", filename);
		return NULL;
	}

	I_Printf("W_AddFile: Adding %s\n", filename);

	startlump = numlumps;

	if (dstricmp(filename + dstrlen(filename) - 3, "wad")) {
		// single lump file

		// fraggle: Swap the filepos and size here.  The WAD directory
		// parsing code expects a little-endian directory, so will swap
		// them back.  Effectively we're constructing a "fake WAD directory"
		// here, as it would appear on disk.

		fileinfo = (filelump_t*)Z_Malloc(sizeof(filelump_t), PU_STATIC, 0);
		fileinfo->filepos = LONG(0);
		fileinfo->size = LONG(wadfile->length);

		// Name the lump after the base of the filename (without the
		// extension).

		ExtractFileBase(filename, fileinfo->name);
		numlumps++;
	}
	else {
		// WAD file
		W_Read(wadfile, 0, &header, sizeof(header));

		if (dstrncmp(header.identification, "PWAD", 4) &&
			dstrncmp(header.identification, "IWAD", 4)) {
			I_Error("W_AddFile: Wad file %s doesn't have valid IWAD or PWAD id\n", filename);
		}

		header.numlumps = LONG(header.numlumps);
		header.infotableofs = LONG(header.infotableofs);
		length = header.numlumps * sizeof(filelump_t);
		fileinfo = (filelump_t*)Z_Malloc(length, PU_STATIC, 0);

		W_Read(wadfile, header.infotableofs, fileinfo, length);
		numlumps += header.numlumps;
	}

	// Fill in lumpinfo
	lumpinfo = (lumpinfo_t*)realloc(lumpinfo, numlumps * sizeof(lumpinfo_t));

	if (lumpinfo == NULL) {
		I_Error("W_AddFile: Couldn't realloc lumpinfo");
	}

	lump_p = &lumpinfo[startlump];

	filerover = fileinfo;

	for (i = startlump; i < numlumps; ++i) {
		lump_p->wadfile = wadfile;
		lump_p->position = LONG(filerover->filepos);
		lump_p->size = LONG(filerover->size);
		lump_p->cache = NULL;
		dmemcpy(lump_p->name, filerover->name, 8);

		++lump_p;
		++filerover;
	}

	Z_Free(fileinfo);

	W_HashLumps();

	return wadfile;
}

//
// LUMP BASED ROUTINES.
//

//
// W_Init
//

void W_Init(void) {
	char* iwad;
	char* doom64expluswad;
	wadinfo_t       header;
	lumpinfo_t* lump_p;
	int             i;
	wad_file_t* wadfile;
	int             length;
	int             startlump;
	filelump_t* fileinfo;
	filelump_t* filerover;
	int             p;

	// open the file and add to directory
	iwad = W_FindIWAD();

	if (iwad == NULL) {
		I_Error("W_Init: IWAD not found");
	}

	wadfile = W_OpenFile(iwad);
    
	W_Read(wadfile, 0, &header, sizeof(header));

	if (dstrnicmp(header.identification, "IWAD", 4)) {
		I_Error("W_Init: Invalid main IWAD id");
	}

	numlumps = 0;
	lumpinfo = (lumpinfo_t*)malloc(1); // Will be realloced as lumps are added

	startlump = numlumps;

	header.numlumps = LONG(header.numlumps);
	header.infotableofs = LONG(header.infotableofs);
	length = header.numlumps * sizeof(filelump_t);
	fileinfo = (filelump_t*)Z_Malloc(length, PU_STATIC, 0);

	W_Read(wadfile, header.infotableofs, fileinfo, length);
	numlumps += header.numlumps;

	// Fill in lumpinfo
	lumpinfo = (lumpinfo_t*)realloc(lumpinfo, numlumps * sizeof(lumpinfo_t));
	dmemset(lumpinfo, 0, numlumps * sizeof(lumpinfo_t));

	if (!lumpinfo) {
		I_Error("W_Init: Couldn't realloc lumpinfo");
	}

	lump_p = &lumpinfo[startlump];
	filerover = fileinfo;

	for (i = startlump; i < numlumps; i++, lump_p++, filerover++) {
		lump_p->wadfile = wadfile;
		lump_p->position = LONG(filerover->filepos);
		lump_p->size = LONG(filerover->size);
		lump_p->cache = NULL;
		dmemcpy(lump_p->name, filerover->name, 8);
	}

	if (!numlumps) {
		I_Error("W_Init: no files found");
	}

	lumpinfo = (lumpinfo_t*)realloc(lumpinfo, numlumps * sizeof(lumpinfo_t));

	Z_Free(fileinfo);

	if ((doom64expluswad = I_FindDataFile("doom64ex-plus.wad"))) {
		W_MergeFile(doom64expluswad);
	}
	else {
		I_Error("W_Init: doom64ex-plus.wad not found");
	}

	p = M_CheckParm("-file");
	if (p)
	{
		// the parms after p are wadfile/lump names,
		// until end of parms or another - preceded parm
		while (++p != myargc && myargv[p][0] != '-')
		{
			char* filename;
			filename = W_TryFindWADByName(myargv[p]);
			W_MergeFile(filename);
		}
	}
	// 20120724 villsa - find drag & drop wad files
	else {
		for (i = 1; i < myargc; i++) {
			if (strstr(myargv[i], ".wad") || strstr(myargv[i], ".WAD")) {
				char* filename;
				if ((filename = W_TryFindWADByName(myargv[i]))) {
					W_MergeFile(filename);
				}
			}
		}
	}

	W_HashLumps();
		
	I_Printf("W_KPFInit: Init KPFfiles.\n");
	Uint64 now = SDL_GetTicks();
	W_KPFInit();
	I_Printf("W_KPFInit: took %d ms\n", SDL_GetTicks() - now);
}

static boolean nonmaplump = false;

filelump_t* mapLump;
int numMapLumps;
byte* mapLumpData = NULL;

//
// W_CacheMapLump
//

void W_CacheMapLump(int map) {
	char name8[9];
	int lump;

	sprintf(name8, "MAP%02d", map);
	name8[8] = 0;

	lump = W_GetNumForName(name8);

	// check if non-lump map, aka standard doom map storage
	if (!((lump + 1) >= numlumps) && !dstrncmp(lumpinfo[lump + 1].name, "THINGS", 8)) {
		nonmaplump = true;
		return;
	}
	else {
		mapLumpData = (byte*)W_CacheLumpNum(lump, PU_STATIC);
	}

	numMapLumps = LONG(((wadinfo_t*)mapLumpData)->numlumps);
	mapLump = (filelump_t*)(mapLumpData + LONG(((wadinfo_t*)mapLumpData)->infotableofs));
}

//
// W_FreeMapLump
//

void W_FreeMapLump(void) {
	if (nonmaplump) {
		Z_FreeTags(PU_MAPLUMP, PU_MAPLUMP);
	}

	mapLump = NULL;
	numMapLumps = 0;
	nonmaplump = false;

	if (mapLumpData) {
		Z_Free(mapLumpData);
	}

	mapLumpData = NULL;
}

//
// W_MapLumpLength
//

int W_MapLumpLength(int lump) {
	if (nonmaplump) {
		char name8[9];
		int l;

		sprintf(name8, "MAP%02d", gamemap);
		name8[8] = 0;

		l = W_GetNumForName(name8);

		return lumpinfo[l + lump].size;
	}

	if (lump >= numMapLumps) {
		I_Error("W_MapLumpLength: %i out of range", lump);
	}

	return LONG(mapLump[lump].size);
}

//
// W_GetMapLump
//

void* W_GetMapLump(int lump) {
	if (nonmaplump) {
		char name8[9];
		int l;

		sprintf(name8, "MAP%02d", gamemap);
		name8[8] = 0;

		l = W_GetNumForName(name8);

		return W_CacheLumpNum(l + lump, PU_MAPLUMP);
	}

	if (lump >= numMapLumps) {
		I_Error("W_GetMapLump: lump %d out of range", lump);
	}

	return (mapLumpData + LONG(mapLump[lump].filepos));
}

//
// W_CheckNumForName
// Returns -1 if name not found.
//

int W_CheckNumForName(const char* name) {
	int i = -1;

	if (numlumps) {
		i = lumpinfo[W_HashLumpName(name) % numlumps].index;
	}

	while (i >= 0 && dstrncmp(lumpinfo[i].name, name, 8)) {
		i = lumpinfo[i].next;
	}

	return i;
}

//
// W_GetNumForName
// Calls W_CheckNumForName, but bombs out if not found.
//

int W_GetNumForName(const char* name) {
	int    i;

	i = W_CheckNumForName(name);

	if (i == -1) {
		I_Warning("W_GetNumForName: %s not found!", name);
	}

	return i;
}

//
// W_LumpLength
// Returns the buffer size needed to load the given lump.
//

int W_LumpLength(int lump) {
	if (lump >= numlumps) {
		I_Error("W_LumpLength: %i >= numlumps", lump);
	}

	return lumpinfo[lump].size;
}

//
// W_ReadLump
// Loads the lump into the given buffer,
//  which must be >= W_LumpLength().
//

void W_ReadLump(int lump, void* dest)
{
	int c;
	lumpinfo_t* l;

	if (lump >= numlumps) {
		I_Error("W_ReadLump: %i >= numlumps", lump);
	}

	l = lumpinfo + lump;

	if (l->wadfile == WADFILE_MEM) {
		int idx = l->position;
		if (idx < 0 || idx >= g_nmemlumps) {
			I_Error("W_ReadLump: bad mem lump index %d", idx);
		}
		dmemcpy(dest, g_memlumps[idx].data, l->size);
		return;
	}

	I_BeginRead();
	c = W_Read(l->wadfile, l->position, dest, l->size);
	if (c < l->size) {
		I_Error("W_ReadLump: only read %i of %i on lump %i", c, l->size, lump);
	}
}

static int W_AddMemoryLump(const char name8[8], unsigned char* data, int size)
{
    if (size <= 0 || data == NULL)
        return 0;

    int existing = W_CheckNumForName(name8);
    if (existing >= 0) {
        if (g_nmemlumps >= MAX_MEMLUMPS) {
            free(data);
            I_Warning("W_AddMemoryLump: out of memlump slots for %8.8s\n", name8);
            return 0;
        }
        int idx = g_nmemlumps++;
        dmemcpy(g_memlumps[idx].name, name8, 8);
        g_memlumps[idx].data = data;
        g_memlumps[idx].size = size;

        lumpinfo_t* L = &lumpinfo[existing];

        if (L->cache) {
            Z_Free(L->cache);
            L->cache = NULL;
        }

        L->wadfile  = WADFILE_MEM;
        L->position = idx;   // index
        L->size     = size;
        dmemcpy(L->name, name8, 8);

        W_HashLumps();

        I_Printf("KPF: Overrode %8.8s in-place with memory lump (%d bytes)\n", name8, size);
        return 1;
    }

    if (g_nmemlumps >= MAX_MEMLUMPS) {
        free(data);
        I_Warning("W_AddMemoryLump: out of memlump slots for %8.8s\n", name8);
        return 0;
    }

    lumpinfo = (lumpinfo_t*)realloc(lumpinfo, (numlumps + 1) * sizeof(lumpinfo_t));
    if (!lumpinfo) {
        free(data);
        I_Error("W_AddMemoryLump: Couldn't realloc lumpinfo");
        return 0;
    }

    int idx = g_nmemlumps++;
    dmemcpy(g_memlumps[idx].name, name8, 8);
    g_memlumps[idx].data = data;
    g_memlumps[idx].size = size;

    lumpinfo_t* L = &lumpinfo[numlumps];
    L->wadfile  = WADFILE_MEM;
    L->position = idx;
    L->size     = size;
    dmemcpy(L->name, name8, 8);
    L->cache    = NULL;

    numlumps++;
    W_HashLumps();
    I_Printf("KPF: Added new memory lump %8.8s (%d bytes)\n", name8, size);
    return 1;
}

void W_KPFInit(void)
{
	static const char* kpf_candidates[] = { "Doom64.kpf" };

	struct override_item {
		char name8[8];
		const char* paths[4];
		int  max_w, max_h;
	};

	// atsb: let's be strict, if these aren't present, then we bail out at 30,000ft without a parachute!
	const struct override_item items[] = {
		{ "TITLE",   { "gfx/Doom64_HiRes.png", NULL }, win_px_w, win_px_h },
		{ "USLEGAL", { "gfx/legals.png",       NULL }, win_px_w, win_px_h },
		{ "CURSOR",  { "gfx/cursor.png",       NULL }, 33, 32 },
	};

	int need = SDL_arraysize(items);
	int loaded = 0;

	char missing[256];
	missing[0] = 0;

	for (size_t it = 0; it < need; ++it) {
		const struct override_item* ov = &items[it];
		int this_ok = 0;

		for (int k = 0; k < SDL_arraysize(kpf_candidates) && !this_ok; ++k) {
			const char* kpf = kpf_candidates[k];

			for (size_t p = 0; ov->paths[p] != NULL && !this_ok; ++p) {
				const char* inner = ov->paths[p];

				unsigned char* data = NULL;
				int size = 0;

				char* kpf_path = I_FindDataFile((char *)kpf);
				if(!kpf_path) continue;
				int ret = KPF_ExtractFileCapped(kpf_path, inner, &data, &size, KPF_PNG_CAP_BYTES);
				if(!ret) continue;

				if (ov->max_w > 0 && ov->max_h > 0) {
					int w = 0, h = 0;
					if (PNG_ReadDimensions(data, (size_t)size, &w, &h)) {
						if (w > ov->max_w || h > ov->max_h) {
							unsigned char* scaled = NULL; int scaled_sz = 0;

							boolean use_cache = true; // can be set to false to disable
							boolean in_cache = false;
							filepath_t cache_filepath;

							if (use_cache) {

								char* cache_dir = I_GetUserFile("kpf_cache");
								if (!I_DirExists(cache_dir)) {
									use_cache = M_CreateDir(cache_dir);
								}

								if (use_cache) {

									SDL_snprintf(cache_filepath, MAX_PATH,
										"%s/%s_%d_%d_%d",
										cache_dir,
										ov->name8,
										ov->max_w, ov->max_h,
										M_FileLengthFromPath(kpf_path));

									if (I_FileExists(cache_filepath)) {

										unsigned char* cache_data;
										int cache_size = M_ReadFileEx(cache_filepath, &cache_data, true);
										in_cache = cache_size > 0;
										if (in_cache) {
											I_Printf("W_KPFInit: %s read from %s\n", ov->name8, cache_filepath);
											free(data);
											data = cache_data;
											size = cache_size;
										}
									}
								}
								free(cache_dir);
							}
							
							if (!use_cache || !in_cache) {
								if (PNG_DownscaleToFit(data, size, ov->max_w, ov->max_h, &scaled, &scaled_sz)) {
									free(data);
									data = scaled;
									size = scaled_sz;
									I_Printf("W_KPFInit: %s too large (%dx%d) scaled to fit %dx%d\n",
										ov->name8, w, h, ov->max_w, ov->max_h);

									if (use_cache) {
										M_WriteFile(cache_filepath, data, size);
									}
								}
								else {
									I_Printf("W_KPFInit: %s too large (%dx%d), keeping WAD version.\n",
										ov->name8, w, h);
									free(data);
									data = NULL;
									size = 0;
								}
							}
						}
					}
				}
				if (data && size > 0) {
					if (W_AddMemoryLump(ov->name8, data, size)) {
						I_Printf("W_KPFInit: Loaded %s from %s (%s, %d bytes)\n", ov->name8, kpf, inner, size);
						this_ok = 1;
						loaded++;
					}
					else {
						I_Printf("W_KPFInit: Failed to add in-memory %s from %s (%s)\n", ov->name8, kpf, inner);
						free(data);
						data = NULL;
					}
				}
			}
		}
		if (!this_ok) {
			size_t cur = strlen(missing);
			size_t left = sizeof(missing) - cur - 1;
			if (left > 0) {
				snprintf(missing + cur, left, "%s%s", cur ? "," : "", ov->name8);
			}
		}
	}
	// atsb: ONLY LOAD IF EVERYTHING IS FOUND!
	if (loaded != need) {
		I_Error("W_KPFInit: required assets missing or corrupt: %s. "
			"Make sure Doom64.kpf is present and readable.", missing[0] ? missing : "(unknown)");
	}
}

//
// W_CacheLumpNum
//

void* W_CacheLumpNum(int lump, int tag) {
	lumpinfo_t* l;

	if (lump < 0 || lump >= numlumps) {
		I_Error("W_CacheLumpNum: lump %i out of range", lump);
	}

	l = &lumpinfo[lump];

	if (!l->cache) {    // read the lump in
		Z_Malloc(W_LumpLength(lump), tag, &l->cache);
		W_ReadLump(lump, l->cache);
	}
	else {
		// [d64] 'touch' static caches
		if (tag == PU_STATIC) {
			Z_Touch(l->cache);
		}

		// avoid changing PU_STATIC data into PU_CACHE
		if (tag < Z_CheckTag(l->cache)) {
			Z_ChangeTag(l->cache, tag);
		}
	}

	return l->cache;
}

//
// W_CacheLumpName
//

void* W_CacheLumpName(const char* name, int tag) {
	return W_CacheLumpNum(W_GetNumForName(name), tag);
}

//
// W_Checksum
//

static wad_file_t** open_wadfiles = NULL;
static int num_open_wadfiles = 0;

static int GetFileNumber(wad_file_t* handle) {
	int i;
	int result;

	for (i = 0; i < num_open_wadfiles; ++i) {
		if (open_wadfiles[i] == handle) {
			return i;
		}
	}

	// Not found in list.  This is a new file we haven't seen yet.
	// Allocate another slot for this file.

	open_wadfiles = (wad_file_t**)realloc(open_wadfiles,
		sizeof(wad_file_t*) * (num_open_wadfiles + 1));
	open_wadfiles[num_open_wadfiles] = handle;

	result = num_open_wadfiles;
	++num_open_wadfiles;

	return result;
}

static void ChecksumAddLump(md5_context_t* md5_context, lumpinfo_t* lump) {
	char buf[9];

	strncpy(buf, lump->name, 8);
	buf[8] = '\0';

	MD5_UpdateString(md5_context, buf);
	MD5_UpdateInt32(md5_context, GetFileNumber(lump->wadfile));
	MD5_UpdateInt32(md5_context, lump->position);
	MD5_UpdateInt32(md5_context, lump->size);
}

void W_Checksum(md5_digest_t digest) {
	md5_context_t md5_context;
	int i;

	MD5_Init(&md5_context);

	num_open_wadfiles = 0;

	// Go through each entry in the WAD directory, adding information
	// about each entry to the MD5 hash.

	for (i = 0; i < numlumps; ++i) {
		ChecksumAddLump(&md5_context, &lumpinfo[i]);
	}

	MD5_Final(digest, &md5_context);
}
