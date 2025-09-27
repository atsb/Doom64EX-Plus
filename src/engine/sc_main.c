// Emacs style mode select   -*- C -*-
//-----------------------------------------------------------------------------
//
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
// DESCRIPTION: Lexter parsing system for script data
//
//-----------------------------------------------------------------------------

#include <stdbool.h>
#include <SDL3/SDL_stdinc.h>

#include "sc_main.h"
#include "doomdef.h"
#include "z_zone.h"
#include "w_wad.h"
#include "m_misc.h"
#include "con_console.h"
#include "i_system.h"

scparser_t sc_parser;

//
// SC_ParseColorValue
//
static int SC_ParseColorValue(const char* token) {
	int len = dstrlen(token);
	if (len >= 2 && token[0] >= '0' && token[0] <= '9') {
		for (int i = 1; i < len; i++) {
			char c = token[i];
			if ((c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f')) {
				char firstDigit[2] = { token[0], '\0' };
				return datoi(firstDigit);
			}
		}
	}

	boolean hasHexLetters = false;
	for (int i = 0; i < len; i++) {
		char c = token[i];
		if ((c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f')) {
			hasHexLetters = true;
			break;
		}
	}

	if (hasHexLetters) {
		return dhtoi((char*)token);
	}

	return datoi(token);
}

//
// SC_Open
//

static void SC_Open(const char* name) {
	int lump;

	CON_DPrintf("--------SC_Open: Reading %s--------\n", name);

	lump = W_CheckNumForName(name);

	if (lump <= -1) {
		sc_parser.buffsize = M_ReadFile((char *)name, &sc_parser.buffer);

		if (sc_parser.buffsize == -1) {
			I_Error("SC_Open: %s not found", name);
		}
	}
	else {
		sc_parser.buffer = W_CacheLumpNum(lump, PU_STATIC);
		sc_parser.buffsize = W_LumpLength(lump);
	}

	CON_DPrintf("%s size: %ikb\n", name, sc_parser.buffsize >> 10);

	sc_parser.pointer_start = sc_parser.buffer;
	sc_parser.pointer_end = sc_parser.buffer + sc_parser.buffsize;
	sc_parser.linepos = 1;
	sc_parser.rowpos = 1;
	sc_parser.buffpos = 0;
}

//
// SC_Close
//

static void SC_Close(void) {
	Z_Free(sc_parser.buffer);

	sc_parser.buffer = NULL;
	sc_parser.buffsize = 0;
	sc_parser.pointer_start = NULL;
	sc_parser.pointer_end = NULL;
	sc_parser.linepos = 0;
	sc_parser.rowpos = 0;
	sc_parser.buffpos = 0;
}

//
// SC_Compare
//

static void SC_Compare(const char* token) {
	sc_parser.find(false);
	if (dstricmp(sc_parser.token, token)) {
		I_Error("SC_Compare: Expected '%s', found '%s' (line = %i, pos = %i)",
			token, sc_parser.token, sc_parser.linepos, sc_parser.rowpos);
	}
}

//
// SC_ReadTokens
//

static int SC_ReadTokens(void) {
	return (sc_parser.buffpos < sc_parser.buffsize);
}

//
// SC_GetString
//

static char* SC_GetString(void) {
	sc_parser.compare("="); // expect a '='
	sc_parser.find(false);  // get next token which should be a string

	return sc_parser.token;
}

//
// SC_GetInteger
//

static int SC_GetInteger(void) {
	sc_parser.compare("="); // expect a '='
	sc_parser.find(false);  // get next token which should be an integer

	return datoi(sc_parser.token);
}

//
// SC_SetData
//

static int SC_SetData(void* data, const scdatatable_t* table) {
	int i;
	boolean ok = false;

	for (i = 0; table[i].token; i++) {
		if (!dstricmp(table[i].token, sc_parser.token)) {
			byte* pointer = ((byte*)data + table[i].ptroffset);
			char* name;
			byte rgb[3];

			ok = true;

			switch (table[i].type) {
			case 's':
				name = sc_parser.getstring();
				dstrncpy((char*)pointer, name, dstrlen(name));
				break;
			case 'S':
				name = sc_parser.getstring();
				dstrupr(name);
				dstrncpy((char*)pointer, name, dstrlen(name));
				break;
			case 'i':
				*(int*)pointer = sc_parser.getint();
				break;
			case 'b':
				*(boolean*)pointer = true;
				break;
			case 'c':
				sc_parser.compare("=");
				sc_parser.find(false);
				rgb[0] = SC_ParseColorValue(sc_parser.token);
				sc_parser.find(false);
				rgb[1] = SC_ParseColorValue(sc_parser.token);
				sc_parser.find(false);
				rgb[2] = SC_ParseColorValue(sc_parser.token);

				*(rcolor*)pointer = D_RGBA(rgb[0], rgb[1], rgb[2], 0xFF);
				break;
			case 'F':
				sc_parser.compare("=");
				sc_parser.find(false);
				rgb[0] = SC_ParseColorValue(sc_parser.token);
				sc_parser.find(false);
				rgb[1] = SC_ParseColorValue(sc_parser.token);
				sc_parser.find(false);
				rgb[2] = SC_ParseColorValue(sc_parser.token);

				*(rcolor*)pointer = rgb[0] | (rgb[1] << 8) | (rgb[2] << 16) | (0xFF << 24);
				break;
			}
			break;
		}
	}
	return ok;
}

//
// SC_Find
//

static int SC_Find(boolean forceupper) {
	char c = 0;
	int i = 0;
	boolean comment = false;
	boolean havetoken = false;
	boolean string = false;

	dmemset(sc_parser.token, 0, 256);

	while (sc_parser.readtokens()) {
		c = sc_parser.fgetchar();

		if (c == '/') {
			comment = true;
		}

		if (comment == false) {
			if (c == '"') {
				if (!string) {
					string = true;
					continue;
				}
				else if (havetoken) {
					c = sc_parser.fgetchar();

					if (c != ',') {
						return true;
					}
					else {
						havetoken = false;
						continue;
					}
				}
				else {
					if (sc_parser.fgetchar() == '"') {
						if (sc_parser.fgetchar() == ',') {
							continue;
						}
						else {
							sc_parser.rewind();
							sc_parser.rewind();
						}
					}
					else {
						sc_parser.rewind();
					}
				}
			}

			if (!string) {
				if (c > ' ') {
					if (havetoken && i > 0) {
						char prev = sc_parser.token[i - 1];
						if (prev >= '0' && prev <= '9' &&
							((c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))) {
							sc_parser.rewind();
							return true;
						}
					}

					havetoken = true;
					sc_parser.token[i++] =
						forceupper ? SDL_toupper(c) : c;
				}
				else if (havetoken) {
					return true;
				}
			}
			else {
				if (c >= ' ' && c != '"') {
					havetoken = true;
					sc_parser.token[i++] =
						forceupper ? SDL_toupper(c) : c;
				}
			}
		}

		if (c == '\n') {
			sc_parser.linepos++;
			sc_parser.rowpos = 1;
			comment = false;
			if (string) {
				sc_parser.token[i++] = c;
			}
		}
	}

	return false;
}

//
// SC_GetChar
//

static char SC_GetChar(void) {
	sc_parser.rowpos++;
	return sc_parser.buffer[sc_parser.buffpos++];
}

//
// SC_Rewind
//

static void SC_Rewind(void) {
	sc_parser.rowpos--;
	sc_parser.buffpos--;
}

//
// SC_Error
//

static void SC_Error(const char* function) {
	if (sc_parser.token[0] < ' ') {
		return;
	}

	I_Warning("%s: Unknown token: '%s' (line = %i, pos = %i)\n",
		function, sc_parser.token, sc_parser.linepos, sc_parser.rowpos);
}

//
// SC_Init
//

void SC_Init(void) {
	//
	// clear variables
	//

	dmemset(&sc_parser, 0, sizeof(scparser_t));

	//
	// setup lexer routines
	//

	sc_parser.open = SC_Open;
	sc_parser.close = SC_Close;
	sc_parser.compare = SC_Compare;
	sc_parser.find = SC_Find;
	sc_parser.fgetchar = SC_GetChar;
	sc_parser.rewind = SC_Rewind;
	sc_parser.getstring = SC_GetString;
	sc_parser.getint = SC_GetInteger;
	sc_parser.setdata = SC_SetData;
	sc_parser.readtokens = SC_ReadTokens;
	sc_parser.error = SC_Error;
}
