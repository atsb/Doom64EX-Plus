#ifndef _STEAM_H_
#define _STEAM_H_

#include "doomtype.h"

#define DOOM64_STEAM_APPID		1148590

#define STEAM_PATH_MAX_SIZE   256

typedef struct steamgame_s {
	int		appid;
	char	*subdir;
	char	library[STEAM_PATH_MAX_SIZE];
} steamgame_t;

boolean	Steam_IsValidPath (const char *path);
boolean	Steam_FindGame (steamgame_t *game, int appid);
boolean Steam_ResolvePath (char *path, size_t pathsize, const steamgame_t *game);

#endif /*_STEAM_H */
