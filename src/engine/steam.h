#ifndef _STEAM_H_
#define _STEAM_H_

#include "doomtype.h"
#include "i_system_io.h"

#define DOOM64_STEAM_APPID		1148590

typedef struct steamgame_s {
	int		    appid;
	char	    *subdir;
	filepath_t	library;
} steamgame_t;

boolean	Steam_IsValidPath (const char *path);
boolean	Steam_FindGame (steamgame_t *game, int appid);
boolean Steam_ResolvePath (filepath_t path, const steamgame_t *game);

#endif /*_STEAM_H */
