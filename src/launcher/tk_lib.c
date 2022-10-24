// Emacs style mode select	 -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: tk_lib.c 337 2009-02-01 21:27:07Z svkaiser $
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// $Author: svkaiser $
// $Revision: 337 $
// $Date: 2009-02-01 21:27:07 +0000 (Sun, 01 Feb 2009) $
//
// DESCRIPTION:
//
//-----------------------------------------------------------------------------
#ifdef RCSID
static const char rcsid[] = "$Id: tk_lib.c 337 2009-02-01 21:27:07Z svkaiser $";
#endif

#include "tk_lib.h"

byte* parse;
unsigned int lastByte;
unsigned int tkPos = 0;
int tkLine = 1;

#define MAXSTRSIZE	512

char stringToken[32];
int intToken;
char dataStrToken[MAXSTROKENS][64];

static int tkStrSlot = 0;

static FILE* tk_file;
char *DefaultConfigFile="launcher.cfg";
static unsigned short dataPos = -1;

void tk_ProcessDefs(tDefTypes_t *def)
{
	switch(def->type)
	{
		case 'i':
			tk_getTokenNum();
			*def->value = intToken;
			break;
		case 's':	//find a quoted string
			tk_getTokenStr();
			*def->strValue = dataStrToken[tkStrSlot++];
			break;
		default:
			L_Complain(L"tk_ProcessDefs: Unknown type: %c", def->type);
			break;
	}
}

void tk_getToken(void)
{
	bool foundToken = false;
	int i = 0;
	memset(&stringToken, 0, 32);
	do
	{
		if(parse[tkPos] == '\n') tkLine++;
		if(parse[tkPos] > 0x20) 
		{
			foundToken = true;
			stringToken[i++] = parse[tkPos];
		}
		else if(foundToken == true) return;
	}	while(++tkPos != lastByte);
}

void tk_getTokenNum(void) 
{
	bool foundToken = false;
	int i = 0, k;
	char text[32];
	intToken = 0;
	do 
	{
		if(parse[tkPos] == '\n') tkLine++;
		if(parse[tkPos] == 45 || (parse[tkPos] >= 48 && parse[tkPos] <= 57)) 
		{
			foundToken = true;
			text[i++] = parse[tkPos];
		}
		else 
		{
			if(foundToken == true) 
			{
				int pow = 1;
				for(k = i-1; k >= 0; k--) 
				{
					(text[k] == 45) ? intToken *= -1 :
					(intToken += (text[k] - 48) * pow);
					pow *= 10;
				}
				return;
			}
			else 
			{
				if(parse[tkPos] > 0x20) 
					L_Complain(L"getTokenNum Line %i: Not a number..\n", tkLine);
			}
		}
	}	while(tkPos++ != lastByte);
}

void tk_getTokenStr(void) 
{
	int i = 0;
	memset(&dataStrToken[tkStrSlot], 0, 64);
	do 
	{
		if(parse[tkPos] == '\n') tkLine++;
		if(parse[tkPos] == '"') 
		{
			tkPos++;
			do 
			{
				if(parse[tkPos] == '\n') 
					L_Complain(L"getTokenStr Line %i: Newline in string..\n", tkLine);

				if(tkPos >= lastByte)
					L_Complain(L"getTokenStr Line %i: End of file before completing string..\n", tkLine);

				dataStrToken[tkStrSlot][i++] = parse[tkPos];
			} while(parse[++tkPos] != '"');
			return;
		}
		else if(!(parse[tkPos] <= 0x20)) L_Complain(L"getTokenStr Line %i: String does not begin with quotes..\n");
	} while(tkPos++ != lastByte);
}

int tk_getTokenLen(void)
{
	int i;
	for(i=0;;i++) if(stringToken[i] == 0) break;
	return i;
}

void tk_toUprToken(void)
{
	int i;
	for(i = 0; i < tk_getTokenLen(); i++) 
		if(stringToken[i] >= 'a' && stringToken[i] <= 'z') stringToken[i] -= 32;
}

void tk_toLwrToken(void)
{
	int i;
	for(i = 0; i < tk_getTokenLen(); i++) 
		if(stringToken[i] >= 'A' && stringToken[i] <= 'Z') stringToken[i] += 32;
}

void tk_FreeParse(void) 
{
	free(parse);
	tkLine = 1;
	tkPos = 0;
	dataPos = -1;
	lastByte = 0;
}

void tk_ResetConfig(tDefTypes_t *cfgdefault)
{
	tDefTypes_t *def;

	for(def = cfgdefault; def->field; def++) 
	{
		if(def->value == NULL) *def->strValue = (char*)def->defaultstring;
		else *def->value = def->defaultvalue;
	}
}

void tk_Open(void)
{
	tk_file = fopen(DefaultConfigFile, "w");
}

void tk_Close(void)
{
	fclose(tk_file);
}

void tk_SaveConfig(tDefTypes_t *cfg)
{
	tDefTypes_t *def;

	for(def = cfg; def->field; def++) 
	{
		if(def->value == NULL) fprintf(tk_file, "%s \"%s\"\n", def->field, *(char**)def->strValue);
		else fprintf(tk_file, "%s %i\n", def->field, *def->value);
	}
}
