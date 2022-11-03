/*tkParse.h (c)2006 Samuel Villarreal*/

#ifndef __TKLIB__
#define __TKLIB__

#include <stdint.h>
#include "Launcher.h"

#define MAXSTROKENS	32

typedef struct
{
	int8_t* field;
	int8_t type;
	int defaultvalue;
	int* value;
	const int8_t* defaultstring;
	int8_t** strValue;
} tDefTypes_t;

extern byte* parse;
extern uint32_t lastByte;
extern uint32_t tkPos;
extern int tkLine;

extern int8_t stringToken[32];
extern int intToken;
int8_t dataStrToken[MAXSTROKENS][64];

extern int8_t* DefaultConfigFile;

void	tk_ProcessDefs(tDefTypes_t* def);
int		tk_getTokenLen(void);
void	tk_getToken(void);
void	tk_getTokenNum(void);
void	tk_getTokenStr(void);
void	tk_FreeParse(void);
void	tk_toUprToken(void);
void	tk_toLwrToken(void);
void	tk_ResetConfig(tDefTypes_t* cfgdefault);
void	tk_SaveConfig(tDefTypes_t* cfg);
void	tk_Open(void);
void	tk_Close(void);

#endif