// Emacs style mode select   -*- C -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 2025 Gibbon
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
//      MAPINFO Implementation for DOOM64 Remastered Support
//
//-----------------------------------------------------------------------------

#include <math.h>
#include <stdlib.h>

#include "p_setup.h"
#include "doomdef.h"
#include "i_swap.h"
#include "m_fixed.h"
#include "g_game.h"
#include "i_system.h"
#include "w_wad.h"
#include "p_local.h"
#include "doomstat.h"
#include "t_bsp.h"
#include "p_macros.h"
#include "info.h"
#include "m_misc.h"
#include "tables.h"
#include "gl_texture.h"
#include "r_sky.h"
#include "r_main.h"
#include "r_lights.h"
#include "r_things.h"
#include "con_console.h"
#include "con_cvar.h"
#include "m_random.h"
#include "z_zone.h"
#include "sc_main.h"

//
// [kex] mapinfo stuff
//

int nummapdef;
mapdef_t* mapdefs;
int numclusterdef;
clusterdef_t* clusterdefs;
int numepisodedef;
episodedef_t* episodedefs;

/* atsb: Everything below this is Clean C24 standard code.
most is traditional UMAPINFO style parsing, nothing major
a few things are D64 specific for remaster style things (double empty lines) 
but nothing UMAPINFO has difficulty in parsing anyway.

So behold, the beautiful parser for remastered MAPINFO lumps
*/

//
// P_InitMapInfo
//

static scdatatable_t mapdatatable[] = {
  {
    "CLASSTYPE",
    (int64_t) & ((mapdef_t*)0)->type,
    'i'
  },
  {
    "LEVELNUM",
    (int64_t) & ((mapdef_t*)0)->mapid,
    'i'
  },
  {
    "CLUSTER",
    (int64_t) & ((mapdef_t*)0)->cluster,
    'i'
  },
  {
    "EXITDELAY",
    (int64_t) & ((mapdef_t*)0)->exitdelay,
    'i'
  },
  {
    "NOINTERMISSION",
    (int64_t) & ((mapdef_t*)0)->nointermission,
    'b'
  },
  {
    "CLEARCHEATS",
    (int64_t) & ((mapdef_t*)0)->clearchts,
    'b'
  },
  {
    "CONTINUEMUSICONEXIT",
    (int64_t) & ((mapdef_t*)0)->contmusexit,
    'b'
  },
  {
    "FORCEGODMODE",
    (int64_t) & ((mapdef_t*)0)->forcegodmode,
    'b'
  },
  {
    NULL,
    0,
    0
  }
};

static scdatatable_t clusterdatatable[] = {
  {
    "PIC",
    (int64_t) & ((clusterdef_t*)0)->pic,
    'S'
  },
  {
    "NOINTERMISSION",
    (int64_t) & ((clusterdef_t*)0)->nointermission,
    'b'
  },
  {
    "SCROLLTEXTEND",
    (int64_t) & ((clusterdef_t*)0)->scrolltextend,
    'b'
  },
  {
    "PIC_X",
    (int64_t) & ((clusterdef_t*)0)->pic_x,
    'i'
  },
  {
    "PIC_Y",
    (int64_t) & ((clusterdef_t*)0)->pic_y,
    'i'
  },
  {
    NULL,
    0,
    0
  }
};

static scdatatable_t episodedatatable[] = {
  {
    "NAME",
    (int64_t) & ((episodedef_t*)0)->name,
    's'
  },
  {
    "KEY",
    (int64_t) & ((episodedef_t*)0)->key,
    's'
  }
};

// Strict MAPINFO/UMAPINFO

typedef enum {
    MI_EOF,
    MI_IDENT,
    MI_STRING,
    MI_NUMBER,
    MI_LBRACE,
    MI_RBRACE,
    MI_EQUALS,
    MI_COMMA
}
mapinfo_token_t;

typedef struct {
    mapinfo_token_t kind;
    const char* start;
    int length;
    long mapinfo_integer_value;
    char* mapinfo_string_value;
    int line, column;
}
mapinfo_token;

typedef struct {
    const char* buf, * cur, * end;
    int line, column;
    mapinfo_token mapinfo_look_ahead;
    const char* filename;
}
mapinfo_lexer;

static void P_MapInfoLexerInit(mapinfo_lexer* mapinfo_lexer,
    const char* data, int length,
    const char* name) {
    mapinfo_lexer->buf = data;
    mapinfo_lexer->cur = data;
    mapinfo_lexer->end = data + length;
    mapinfo_lexer->line = 1;
    mapinfo_lexer->column = 1;
    mapinfo_lexer->mapinfo_look_ahead.kind = MI_EOF;
    mapinfo_lexer->filename = name ? name : "MAPINFO";
}

static int P_MapInfoLexerPeek(mapinfo_lexer* mapinfo_lexer) {
    return (mapinfo_lexer->cur < mapinfo_lexer->end) ? (unsigned char)*mapinfo_lexer->cur : 0;
}
static int P_MapInfoLexerGet(mapinfo_lexer* mapinfo_lexer) {
    if (mapinfo_lexer->cur >= mapinfo_lexer->end) 
        return 0;
    int c = (unsigned char)*mapinfo_lexer->cur++;
    if (c == '\n') {
        mapinfo_lexer->line++;
        mapinfo_lexer->column = 1;
    }
    else mapinfo_lexer->column++;
    return c;
}
static void P_MapInfoLexerSkip(mapinfo_lexer* mapinfo_lexer) {
    for (;;) {
        int c = P_MapInfoLexerPeek(mapinfo_lexer);
        while (c && (c == ' ' || c == '\t' || c == '\r' || c == '\n')) {
            P_MapInfoLexerGet(mapinfo_lexer);
            c = P_MapInfoLexerPeek(mapinfo_lexer);
        }
        if (c == '/' && (mapinfo_lexer->cur + 1) < mapinfo_lexer->end && mapinfo_lexer->cur[1] == '/') {
            while ((c = P_MapInfoLexerGet(mapinfo_lexer)) && c != '\n') {}
            continue;
        }
        if (c == '/' && (mapinfo_lexer->cur + 1) < mapinfo_lexer->end && mapinfo_lexer->cur[1] == '*') {
            P_MapInfoLexerGet(mapinfo_lexer);
            P_MapInfoLexerGet(mapinfo_lexer);
            int p = 0;
            while ((c = P_MapInfoLexerGet(mapinfo_lexer))) {
                if (p == '*' && c == '/') 
                    break;
                p = c;
            }
            continue;
        }
        break;
    }
}

static char* P_MapInfoLexerReadString(mapinfo_lexer* mapinfo_lexer) {
    char* out = NULL;
    size_t cap = 0, n = 0;
    for (;;) {
        int c = P_MapInfoLexerGet(mapinfo_lexer);
        if (!c) {
            CON_Warnf("%s:%d:%d: unterminated string\n", mapinfo_lexer->filename, mapinfo_lexer->line, mapinfo_lexer->column);
            break;
        }
        if (c == '"') 
            break;
        if (c == '\\') {
            int e = P_MapInfoLexerGet(mapinfo_lexer);
            if (!e) 
                break;
            switch (e) {
            case 'n':
                c = '\n';
                break;
            case 'r':
                c = '\r';
                break;
            case 't':
                c = '\t';
                break;
            case '"':
                c = '"';
                break;
            case '\\':
                c = '\\';
                break;
            default:
                c = e;
                break;
            }
        }
        if (n + 1 >= cap) {
            cap = cap ? cap * 2 : 32;
            out = (char*)Z_Realloc(out, cap, PU_STATIC, 0);
        }
        out[n++] = (char)c;
    }
    if (n + 1 >= cap) {
        cap = cap ? cap * 2 : 32;
        out = (char*)Z_Realloc(out, cap, PU_STATIC, 0);
    }
    out[n] = 0;
    return out ? out : (char*)
        "";
}

static mapinfo_token P_MapInfoLexerNextRaw(mapinfo_lexer* mapinfo_lexer) {
    P_MapInfoLexerSkip(mapinfo_lexer);
    mapinfo_token t;
    dmemset(&t, 0, sizeof(t));
    t.line = mapinfo_lexer->line;
    t.column = mapinfo_lexer->column;
    int c = P_MapInfoLexerGet(mapinfo_lexer);
    if (!c) {
        t.kind = MI_EOF;
        return t;
    }

    if (isalpha(c) || c == '_') {
        const char* s = mapinfo_lexer->cur - 1;
        while (mapinfo_lexer->cur < mapinfo_lexer->end && (isalnum((unsigned char)*mapinfo_lexer->cur) || *mapinfo_lexer->cur == '_')) 
            P_MapInfoLexerGet(mapinfo_lexer);
        t.kind = MI_IDENT;
        t.start = s;
        t.length = (int)(mapinfo_lexer->cur - s);
        t.mapinfo_string_value = (char*)Z_Malloc(t.length + 1, PU_STATIC, 0);
        for (int i = 0; i < t.length; i++) t.mapinfo_string_value[i] = (char)tolower((unsigned char)s[i]);
        t.mapinfo_string_value[t.length] = 0;
        return t;
    }
    if (isdigit(c) || (c == '-' && isdigit(P_MapInfoLexerPeek(mapinfo_lexer)))) {
        const char* s = mapinfo_lexer->cur - 1;
        while (isdigit(P_MapInfoLexerPeek(mapinfo_lexer))) P_MapInfoLexerGet(mapinfo_lexer);
        t.kind = MI_NUMBER;
        t.start = s;
        t.length = (int)(mapinfo_lexer->cur - s);
        t.mapinfo_integer_value = strtol(s, NULL, 10);
        return t;
    }
    if (c == '"') {
        t.kind = MI_STRING;
        t.mapinfo_string_value = P_MapInfoLexerReadString(mapinfo_lexer);
        return t;
    }

    switch (c) {
    case '{':
        t.kind = MI_LBRACE;
        break;
    case '}':
        t.kind = MI_RBRACE;
        break;
    case '=':
        t.kind = MI_EQUALS;
        break;
    case ',':
        t.kind = MI_COMMA;
        break;
    default:
        CON_Warnf("%s:%d:%d: skipping unexpected character '%c'\n", mapinfo_lexer->filename, t.line, t.column, c);
        return P_MapInfoLexerNextRaw(mapinfo_lexer);
    }
    return t;
}
static mapinfo_token P_MapInfoLexerPeekLong(mapinfo_lexer* mapinfo_lexer) {
    if (mapinfo_lexer->mapinfo_look_ahead.kind != MI_EOF) 
        
        return mapinfo_lexer->mapinfo_look_ahead;
    mapinfo_lexer->mapinfo_look_ahead = P_MapInfoLexerNextRaw(mapinfo_lexer);

    return mapinfo_lexer->mapinfo_look_ahead;
}
static mapinfo_token P_MapInfoLexerNext(mapinfo_lexer* mapinfo_lexer) {
    if (mapinfo_lexer->mapinfo_look_ahead.kind != MI_EOF) {
        mapinfo_token t = mapinfo_lexer->mapinfo_look_ahead;
        mapinfo_lexer->mapinfo_look_ahead.kind = MI_EOF;

        return t;
    }
    return P_MapInfoLexerNextRaw(mapinfo_lexer);
}
static boolean P_MapInfoLexerAccept(mapinfo_lexer* mapinfo_lexer, mapinfo_token_t k) {
    if (P_MapInfoLexerPeekLong(mapinfo_lexer).kind == k) {
        P_MapInfoLexerNext(mapinfo_lexer);

        return true;
    }
    return false;
}
static void P_MapInfoLexerExpect(mapinfo_lexer* mapinfo_lexer, mapinfo_token_t k,
    const char* what) {
    if (!P_MapInfoLexerAccept(mapinfo_lexer, k)) {
        mapinfo_token lex_acpt = P_MapInfoLexerPeekLong(mapinfo_lexer);
        CON_Warnf("%s:%d:%d: expected %s\n", mapinfo_lexer->filename, lex_acpt.line, lex_acpt.column, what);
    }
}
static void P_MapInfoLexerRightBrace(mapinfo_lexer* mapinfo_lexer) {
    int depth = 0;
    for (;;) {
        mapinfo_token r_brace = P_MapInfoLexerNext(mapinfo_lexer);
        if (r_brace.kind == MI_EOF) 
            return;

        if (r_brace.kind == MI_LBRACE) depth++;
        else if (r_brace.kind == MI_RBRACE) {
            if (depth == 0) 
                return;

            else depth--;
        }
    }
}

static int P_MapInfoLexerParseIntAfterEq(mapinfo_lexer* mapinfo_lexer,
    const char* key) {
    P_MapInfoLexerExpect(mapinfo_lexer, MI_EQUALS, "'='");
    mapinfo_token mapinfo_int_after_value = P_MapInfoLexerNext(mapinfo_lexer);
    if (mapinfo_int_after_value.kind != MI_NUMBER) {
        CON_Warnf("%s:%d:%d: %s requires integer\n", mapinfo_lexer->filename, 
            mapinfo_int_after_value.line, mapinfo_int_after_value.column, key);

        return 0;
    }
    return (int)mapinfo_int_after_value.mapinfo_integer_value;
}
static
const char* P_MapInfoLexerParseStrAfterEq(mapinfo_lexer* mapinfo_lexer,
    const char* key) {
    P_MapInfoLexerExpect(mapinfo_lexer, MI_EQUALS, "'='");
    mapinfo_token mapinfo_string_after_value = P_MapInfoLexerNext(mapinfo_lexer);
    if (mapinfo_string_after_value.kind == MI_STRING) 
        return mapinfo_string_after_value.mapinfo_string_value;

    if (mapinfo_string_after_value.kind == MI_IDENT) 
        return mapinfo_string_after_value.mapinfo_string_value;

    CON_Warnf("%s:%d:%d: %s requires string\n", mapinfo_lexer->filename, mapinfo_string_after_value.line, mapinfo_string_after_value.column, key);
    return "";
}
static void P_MapInfoLexerParseStringList(mapinfo_lexer* mapinfo_lexer, char* outBuf, size_t outCap) {
    outBuf[0] = 0;
    for (;;) {
        mapinfo_token string_v = P_MapInfoLexerNext(mapinfo_lexer);
        if (string_v.kind != MI_STRING) {
            CON_Warnf("%s:%d:%d: expected string in list\n", mapinfo_lexer->filename, string_v.line, string_v.column);
            break;
        }
        if (outBuf[0]) strncat(outBuf, "\n", outCap);
        strncat(outBuf, string_v.mapinfo_string_value, outCap);
        if (!P_MapInfoLexerAccept(mapinfo_lexer, MI_COMMA)) 
            break;
        if (P_MapInfoLexerPeekLong(mapinfo_lexer).kind != MI_STRING) {
            mapinfo_token comma = P_MapInfoLexerPeekLong(mapinfo_lexer);
            CON_Warnf("%s:%d:%d: trailing comma in string list (allowed)\n", mapinfo_lexer->filename, comma.line, comma.column);
            break;
        }
    }
}

static int P_MapInfoLexerResolveMapMusic(const char* name) {
    if (!name || !*name) return -1;

    const int lump = W_CheckNumForName(name);
    if (lump == -1) {
        CON_Warnf("P_InitMapInfo: Invalid music name: %s\n", name);
        return -1;
    }

    return lump;
}

static int P_MapInfoLexerResolveClusterMusic(const char* name) {
    if (!name || !*name) 
        return -1;
    int lump = W_CheckNumForName(name);
    if (lump == -1) {
        CON_Warnf("P_InitMapInfo: Invalid music name: %s\n", name);
        return -1;
    }
    return lump;
}

static void P_MapInfoLexerParseMapBlock(mapinfo_lexer* mapinfo_lexer,
    const char* lumpName,
    const char* title) {
    mapdef_t mapdef;
    dmemset(&mapdef, 0, sizeof(mapdef));

    mapdef.mapid = 1;
    mapdef.exitdelay = 15;
    mapdef.music = -1;

    if (title && *title) {
        dstrncpy(mapdef.mapname, title, dstrlen(title));
    }
    else if (lumpName && *lumpName) {
        dstrncpy(mapdef.mapname, lumpName, dstrlen(lumpName));
    }

    for (;;) {
        mapinfo_token mapinfo_map_value = P_MapInfoLexerNext(mapinfo_lexer);
        if (mapinfo_map_value.kind == MI_RBRACE) break;
        if (mapinfo_map_value.kind == MI_EOF) {
            CON_Warnf("%s:%d:%d: unexpected EOF in map block\n", mapinfo_lexer->filename, 
                mapinfo_map_value.line, mapinfo_map_value.column);
            break;
        }
        if (mapinfo_map_value.kind != MI_IDENT) {
            CON_Warnf("%s:%d:%d: expected key in map block\n", mapinfo_lexer->filename, 
                mapinfo_map_value.line, mapinfo_map_value.column);
            P_MapInfoLexerRightBrace(mapinfo_lexer);
            break;
        }

        /* numeric keys */
        if (!dstricmp(mapinfo_map_value.mapinfo_string_value, "classtype")) {
            mapdef.type = P_MapInfoLexerParseIntAfterEq(mapinfo_lexer, "classtype");

            continue;
        }
        if (!dstricmp(mapinfo_map_value.mapinfo_string_value, "levelnum")) {
            mapdef.mapid = P_MapInfoLexerParseIntAfterEq(mapinfo_lexer, "levelnum");

            continue;
        }
        if (!dstricmp(mapinfo_map_value.mapinfo_string_value, "cluster")) {
            mapdef.cluster = P_MapInfoLexerParseIntAfterEq(mapinfo_lexer, "cluster");

            continue;
        }
        if (!dstricmp(mapinfo_map_value.mapinfo_string_value, "exitdelay")) {
            mapdef.exitdelay = P_MapInfoLexerParseIntAfterEq(mapinfo_lexer, "exitdelay");

            continue;
        }
        if (!dstricmp(mapinfo_map_value.mapinfo_string_value, "compat_collision")) {
            mapdef.compat_collision = P_MapInfoLexerParseIntAfterEq(mapinfo_lexer, "compat_collision");

            continue;
        }
        if (!dstricmp(mapinfo_map_value.mapinfo_string_value, "allowfreelook")) {
            int v = P_MapInfoLexerParseIntAfterEq(mapinfo_lexer, "allowfreelook");
            if (v == 1) mapdef.allowfreelook = 1;
            else if (v == 0) mapdef.allowfreelook = 2;
            else CON_Warnf("%s:%d:%d: allowfreelook should be 0 or 1\n", mapinfo_lexer->filename, 
                mapinfo_map_value.line, mapinfo_map_value.column);

            continue;
        }
        if (!dstricmp(mapinfo_map_value.mapinfo_string_value, "music")) {
            const char* nm = P_MapInfoLexerParseStrAfterEq(mapinfo_lexer, "music");
            mapdef.music = P_MapInfoLexerResolveMapMusic(nm);

            continue;
        }

        /* flags */
        if (!dstricmp(mapinfo_map_value.mapinfo_string_value, "nointermission")) {
            mapdef.nointermission = 1;

            continue;
        }
        if (!dstricmp(mapinfo_map_value.mapinfo_string_value, "clearcheats")) {
            mapdef.clearchts = 1;

            continue;
        }
        if (!dstricmp(mapinfo_map_value.mapinfo_string_value, "continuemusiconexit")) {
            mapdef.contmusexit = 1;

            continue;
        }
        if (!dstricmp(mapinfo_map_value.mapinfo_string_value, "forcegodmode")) {
            mapdef.forcegodmode = 1;

            continue;
        }

        CON_Warnf("%s:%d:%d: unknown map key '%s' (skipping)\n", mapinfo_lexer->filename, 
            mapinfo_map_value.line, mapinfo_map_value.column, mapinfo_map_value.mapinfo_string_value);

        if (P_MapInfoLexerAccept(mapinfo_lexer, MI_EQUALS)) {
            mapinfo_token mapinfo_map_value_long = P_MapInfoLexerPeekLong(mapinfo_lexer);

            if (mapinfo_map_value_long.kind == MI_STRING || mapinfo_map_value_long.kind == MI_NUMBER || mapinfo_map_value_long.kind == MI_IDENT) 
                P_MapInfoLexerNext(mapinfo_lexer);
            else if (mapinfo_map_value_long.kind == MI_LBRACE) P_MapInfoLexerRightBrace(mapinfo_lexer);
        }
    }

    mapdefs = Z_Realloc(mapdefs, sizeof(mapdef_t) * ++nummapdef, PU_STATIC, 0);
    dmemcpy(&mapdefs[nummapdef - 1], &mapdef, sizeof(mapdef_t));
}

// map cluster blocks
static void P_MapInfoLexerParseClusterBlock(mapinfo_lexer* mapinfo_lexer, int id) {
    clusterdef_t c;
    dmemset(&c, 0, sizeof(c));
    c.id = id;

    for (;;) {
        mapinfo_token mapinfo_cluster_value = P_MapInfoLexerNext(mapinfo_lexer);
        if (mapinfo_cluster_value.kind == MI_RBRACE) break;
        if (mapinfo_cluster_value.kind == MI_EOF) {
            CON_Warnf("%s:%d:%d: unexpected EOF in cluster block\n", mapinfo_lexer->filename, 
                mapinfo_cluster_value.line, mapinfo_cluster_value.column);
            break;
        }
        if (mapinfo_cluster_value.kind != MI_IDENT) {
            CON_Warnf("%s:%d:%d: expected key in cluster block\n", mapinfo_lexer->filename, 
                mapinfo_cluster_value.line, mapinfo_cluster_value.column);
            P_MapInfoLexerRightBrace(mapinfo_lexer);
            break;
        }

        if (!dstricmp(mapinfo_cluster_value.mapinfo_string_value, "pic")) {
            const char* v = P_MapInfoLexerParseStrAfterEq(mapinfo_lexer, "pic");
            dstrncpy(c.pic, v, dstrlen(v));

            continue;
        }
        if (!dstricmp(mapinfo_cluster_value.mapinfo_string_value, "pic_x")) {
            c.pic_x = P_MapInfoLexerParseIntAfterEq(mapinfo_lexer, "pic_x");

            continue;
        }
        if (!dstricmp(mapinfo_cluster_value.mapinfo_string_value, "pic_y")) {
            c.pic_y = P_MapInfoLexerParseIntAfterEq(mapinfo_lexer, "pic_y");

            continue;
        }
        if (!dstricmp(mapinfo_cluster_value.mapinfo_string_value, "music")) {
            const char* v = P_MapInfoLexerParseStrAfterEq(mapinfo_lexer, "music");
            c.music = P_MapInfoLexerResolveClusterMusic(v);

            continue;
        }
        if (!dstricmp(mapinfo_cluster_value.mapinfo_string_value, "scrolltextend")) {
            c.scrolltextend = 1;

            continue;
        }
        if (!dstricmp(mapinfo_cluster_value.mapinfo_string_value, "nointermission")) {
            c.nointermission = 1;

            continue;
        }

        if (!dstricmp(mapinfo_cluster_value.mapinfo_string_value, "entertext")) {
            char tmp[4096];
            dmemset(tmp, 0, sizeof(tmp));
            P_MapInfoLexerExpect(mapinfo_lexer, MI_EQUALS, "'='");
            P_MapInfoLexerParseStringList(mapinfo_lexer, tmp, sizeof(tmp));
            c.enteronly = true;
            dstrncpy(c.text, tmp, dstrlen(tmp));

            continue;
        }
        if (!dstricmp(mapinfo_cluster_value.mapinfo_string_value, "exittext")) {
            char tmp[4096];
            dmemset(tmp, 0, sizeof(tmp));
            P_MapInfoLexerExpect(mapinfo_lexer, MI_EQUALS, "'='");
            P_MapInfoLexerParseStringList(mapinfo_lexer, tmp, sizeof(tmp));
            dstrncpy(c.text, tmp, dstrlen(tmp));

            continue;
        }

        CON_Warnf("%s:%d:%d: unknown cluster key '%s' (skipping)\n", mapinfo_lexer->filename, 
            mapinfo_cluster_value.line, mapinfo_cluster_value.column, mapinfo_cluster_value.mapinfo_string_value);
        if (P_MapInfoLexerAccept(mapinfo_lexer, MI_EQUALS)) {
            mapinfo_token mapinfo_cluster_value_long = P_MapInfoLexerPeekLong(mapinfo_lexer);
            if (mapinfo_cluster_value_long.kind == MI_STRING || mapinfo_cluster_value_long.kind == MI_NUMBER || mapinfo_cluster_value_long.kind == MI_IDENT) 
                P_MapInfoLexerNext(mapinfo_lexer);
            else if (mapinfo_cluster_value_long.kind == MI_LBRACE) P_MapInfoLexerRightBrace(mapinfo_lexer);
        }
    }
    clusterdefs = Z_Realloc(clusterdefs, sizeof(clusterdef_t) * ++numclusterdef, PU_STATIC, 0);
    dmemcpy(&clusterdefs[numclusterdef - 1], &c, sizeof(clusterdef_t));
}

static void P_MapInfoLexerParseEpisodeBlock(mapinfo_lexer* mapinfo_lexer, int startmap) {
    episodedef_t e;
    dmemset(&e, 0, sizeof(e));
    e.mapid = startmap;

    for (;;) {
        mapinfo_token mapinfo_episode_value = P_MapInfoLexerNext(mapinfo_lexer);
        if (mapinfo_episode_value.kind == MI_RBRACE) break;
        if (mapinfo_episode_value.kind == MI_EOF) {
            CON_Warnf("%s:%d:%d: unexpected EOF in episode block\n", mapinfo_lexer->filename, 
                mapinfo_episode_value.line, mapinfo_episode_value.column);
            break;
        }
        if (mapinfo_episode_value.kind != MI_IDENT) {
            CON_Warnf("%s:%d:%d: expected key in episode block\n", mapinfo_lexer->filename, 
                mapinfo_episode_value.line, mapinfo_episode_value.column);
            P_MapInfoLexerRightBrace(mapinfo_lexer);
            break;
        }


        // atsb: name and key need to be bounded as these are single char names (episode select) and can overflow
        if (!dstricmp(mapinfo_episode_value.mapinfo_string_value, "name")) {
            const char* v = P_MapInfoLexerParseStrAfterEq(mapinfo_lexer, "name");
            {
                size_t cap = sizeof(e.name);
                size_t n = dstrlen(v);
                if (cap) {
                    if (n >= cap) n = cap - 1;
                    dmemcpy(e.name, v, n);
                    e.name[n] = 0;
                }
            }
            continue;
        }

        if (!dstricmp(mapinfo_episode_value.mapinfo_string_value, "key")) {
            const char* v = P_MapInfoLexerParseStrAfterEq(mapinfo_lexer, "key");
            if (sizeof(e.key) == 1) {
                ((unsigned char*)&e.key)[0] = (v && v[0]) ? (unsigned char)v[0] : 0;
            }
            else {
                size_t cap = sizeof(e.key);
                size_t n = dstrlen(v);
                if (cap) {
                    if (n >= cap) n = cap - 1;
                    dmemcpy(e.key, v, n);
                    ((char*)e.key)[n] = 0;
                }
            }
            continue;
        }

        CON_Warnf("%s:%d:%d: unknown episode key '%s' (skipping)\n", mapinfo_lexer->filename, 
            mapinfo_episode_value.line, mapinfo_episode_value.column, mapinfo_episode_value.mapinfo_string_value);
        if (P_MapInfoLexerAccept(mapinfo_lexer, MI_EQUALS)) {
            mapinfo_token mapinfo_episode_value_long = P_MapInfoLexerPeekLong(mapinfo_lexer);
            if (mapinfo_episode_value_long.kind == MI_STRING || mapinfo_episode_value_long.kind == MI_NUMBER || mapinfo_episode_value_long.kind == MI_IDENT) 
                P_MapInfoLexerNext(mapinfo_lexer);

            else if (mapinfo_episode_value_long.kind == MI_LBRACE) P_MapInfoLexerRightBrace(mapinfo_lexer);
        }
    }
    episodedefs = Z_Realloc(episodedefs, sizeof(episodedef_t) * ++numepisodedef, PU_STATIC, 0);
    dmemcpy(&episodedefs[numepisodedef - 1], &e, sizeof(e));
}

static void P_MapInfoLexerParseTop(mapinfo_lexer* mapinfo_lexer) {
    for (;;) {
        mapinfo_token t = P_MapInfoLexerPeekLong(mapinfo_lexer);
        if (t.kind == MI_EOF) break;
        if (t.kind == MI_IDENT) {
            if (!dstricmp(t.mapinfo_string_value, "map")) {
                P_MapInfoLexerNext(mapinfo_lexer);

                mapinfo_token mapinfo_lump_value = P_MapInfoLexerNext(mapinfo_lexer);
                const char* lump = NULL;
                if (mapinfo_lump_value.kind == MI_IDENT || mapinfo_lump_value.kind == MI_STRING) lump = mapinfo_lump_value.mapinfo_string_value;
                else {
                    CON_Warnf("%s:%d:%d: expected lump name after 'map'\n", mapinfo_lexer->filename, mapinfo_lump_value.line, mapinfo_lump_value.column);
                    P_MapInfoLexerRightBrace(mapinfo_lexer);
                    continue;
                }

                const char* title = NULL;
                if (P_MapInfoLexerPeekLong(mapinfo_lexer).kind == MI_STRING) title = P_MapInfoLexerNext(mapinfo_lexer).mapinfo_string_value;
                P_MapInfoLexerExpect(mapinfo_lexer, MI_LBRACE, "'{' to open map block");
                P_MapInfoLexerParseMapBlock(mapinfo_lexer, lump, title);

                continue;
            }
            if (!dstricmp(t.mapinfo_string_value, "cluster")) {
                P_MapInfoLexerNext(mapinfo_lexer);
                mapinfo_token mapinfo_id = P_MapInfoLexerNext(mapinfo_lexer);
                if (mapinfo_id.kind != MI_NUMBER) {
                    CON_Warnf("%s:%d:%d: expected numeric id after 'cluster'\n", mapinfo_lexer->filename, mapinfo_id.line, mapinfo_id.column);
                    P_MapInfoLexerRightBrace(mapinfo_lexer);

                    continue;
                }
                P_MapInfoLexerExpect(mapinfo_lexer, MI_LBRACE, "'{' to open cluster block");
                P_MapInfoLexerParseClusterBlock(mapinfo_lexer, (int)mapinfo_id.mapinfo_integer_value);

                continue;
            }
            if (!dstricmp(t.mapinfo_string_value, "episode")) {
                P_MapInfoLexerNext(mapinfo_lexer);
                mapinfo_token mapinfo_id = P_MapInfoLexerNext(mapinfo_lexer);
                int startmap = 0;
                if (mapinfo_id.kind == MI_NUMBER) startmap = (int)mapinfo_id.mapinfo_integer_value;
                else {
                    CON_Warnf("%s:%d:%d: expected start map number after 'episode'\n", mapinfo_lexer->filename, mapinfo_id.line, mapinfo_id.column);
                }
                P_MapInfoLexerExpect(mapinfo_lexer, MI_LBRACE, "'{' to open episode block");
                P_MapInfoLexerParseEpisodeBlock(mapinfo_lexer, startmap);

                continue;
            }
        }

        CON_Warnf("%s:%d:%d: unexpected token at top-level; skipping\n", mapinfo_lexer->filename, t.line, t.column);
        P_MapInfoLexerNext(mapinfo_lexer);
    }
}

// entry point
void P_InitMapInfo(void) {
    mapdefs = NULL;
    clusterdefs = NULL;
    episodedefs = NULL;
    nummapdef = 0;
    numclusterdef = 0;
    numepisodedef = 0;

    int lump = W_CheckNumForName("MAPINFO");
    if (lump == -1) {
        CON_DPrintf("No MAPINFO lump found; 0 map/cluster/episode defs\n");
        return;
    }

    int length = W_LumpLength(lump);
    const char* raw = (const char*)W_CacheLumpNum(lump, PU_STATIC);
    char* buf = (char*)Z_Malloc(length + 1, PU_STATIC, 0);
    dmemcpy(buf, raw, length);
    buf[length] = 0;

    mapinfo_lexer mapinfo_lexer;
    P_MapInfoLexerInit(&mapinfo_lexer, buf, length, "MAPINFO");

    P_MapInfoLexerParseTop(&mapinfo_lexer);

    CON_DPrintf("%i map definitions\n", nummapdef);
    CON_DPrintf("%i cluster definitions\n", numclusterdef);
    CON_DPrintf("%i episode definitions\n", numepisodedef);
}
