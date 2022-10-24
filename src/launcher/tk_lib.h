/*tkParse.h (c)2006 Samuel Villarreal*/

#ifndef __TKLIB__
#define __TKLIB__

#include "Launcher.h"

#define MAXSTROKENS	32

typedef struct 
{
	char* field;
	char type;
	int defaultvalue;
	int *value;
	const char *defaultstring;
	char** strValue;
} tDefTypes_t;

extern byte* parse;
extern unsigned int lastByte;
extern unsigned int tkPos;
extern int tkLine;

extern char stringToken[32];
extern int intToken;
char dataStrToken[MAXSTROKENS][64];

extern char *DefaultConfigFile;

void	tk_ProcessDefs(tDefTypes_t *def);
int		tk_getTokenLen(void);
void	tk_getToken(void);
void	tk_getTokenNum(void);
void	tk_getTokenStr(void);
void	tk_FreeParse(void);
void	tk_toUprToken(void);
void	tk_toLwrToken(void);
void	tk_ResetConfig(tDefTypes_t *cfgdefault);
void	tk_SaveConfig(tDefTypes_t *cfg);
void	tk_Open(void);
void	tk_Close(void);

#endif