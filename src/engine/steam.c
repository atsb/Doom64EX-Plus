#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <SDL3/SDL_platform_defines.h>

#ifdef SDL_PLATFORM_LINUX
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#endif

#include "i_system.h"
#include "steam.h"


typedef struct vdbcontext_s vdbcontext_t;
typedef void (*vdbcallback_t) (vdbcontext_t *ctx, const char *key, const char *value);

struct vdbcontext_s
{
	void			*userdata;
	vdbcallback_t	callback;
	int				depth;
	const char		*path[256];
};


/* HELPERS */

#define countof(arr) (sizeof(arr) / sizeof(arr[0]))

static inline int q_isspace(int c)
{
    switch(c) {
    case ' ':  case '\t':
    case '\n': case '\r':
    case '\f': case '\v': return 1;
    }
    return 0;
}


long COM_filelength (FILE *f)
{
	long		pos, end;

	pos = ftell (f);
	fseek (f, 0, SEEK_END);
	end = ftell (f);
	fseek (f, pos, SEEK_SET);

	return end;
}

byte *COM_LoadMallocFile_TextMode_OSPath (const char *path, long *len_out)
{
	FILE	*f;
	byte	*data;
	long	len, actuallen;
	
	f = fopen (path, "rt");
	if (f == NULL)
		return NULL;
	
	len = COM_filelength (f);
	if (len < 0)
		return NULL;
	
	data = (byte *) malloc (len + 1);
	if (data == NULL)
		return NULL;

	// (actuallen < len) if CRLF to LF translation was performed
	actuallen = fread (data, 1, len, f);
	if (ferror(f))
	{
		free (data);
		return NULL;
	}
	data[actuallen] = '\0';
	
	if (len_out != NULL)
		*len_out = actuallen;
	return data;
}


boolean Sys_GetSteamDir (char *path, size_t pathsize)
{
#ifdef SDL_PLATFORM_LINUX

    const char		*home_dir = NULL;
	struct passwd	*pwent;

	pwent = getpwuid( getuid() );
	if (pwent == NULL)
		perror("getpwuid");
	else
		home_dir = pwent->pw_dir;
	if (home_dir == NULL)
		home_dir = getenv("HOME");
	if (home_dir == NULL)
		return false;

	if ((size_t) snprintf (path, pathsize, "%s/.steam/steam", home_dir) < pathsize && Steam_IsValidPath (path))
		return true;
	if ((size_t) snprintf (path, pathsize, "%s/.local/share/Steam", home_dir) < pathsize && Steam_IsValidPath (path))
		return true;
	if ((size_t) snprintf (path, pathsize, "%s/.var/app/com.valvesoftware.Steam/.steam/steam", home_dir) < pathsize && Steam_IsValidPath (path))
		return true;
	if ((size_t) snprintf (path, pathsize, "%s/.var/app/com.valvesoftware.Steam/.local/share/Steam", home_dir) < pathsize && Steam_IsValidPath (path))
		return true;

	return false;

#elif defined(SDL_PLATFORM_WIN32)
    return I_GetRegistryString (HKEY_CURRENT_USER, L"Software\\Valve\\Steam", L"SteamPath", path, pathsize);    
#else
    return false;
#endif    
    
}


/*
========================
VDB_ParseString

Parses a quoted string (potentially with escape sequences)
========================
*/
static char *VDB_ParseString (char **buf)
{
	char *ret, *write;
	while (q_isspace (**buf))
		++*buf;

	if (**buf != '"')
		return NULL;

	write = ret = ++*buf;
	for (;;)
	{
		if (!**buf) // premature end of buffer
			return NULL;

		if (**buf == '\\') // escape sequence
		{
			++*buf;
			switch (**buf)
			{
				case '\'':	*write++ = '\''; break;
				case '"':	*write++ = '"';  break;
				case '?':	*write++ = '\?'; break;
				case '\\':	*write++ = '\\'; break;
				case 'a':	*write++ = '\a'; break;
				case 'b':	*write++ = '\b'; break;
				case 'f':	*write++ = '\f'; break;
				case 'n':	*write++ = '\n'; break;
				case 'r':	*write++ = '\r'; break;
				case 't':	*write++ = '\t'; break;
				case 'v':	*write++ = '\v'; break;
				default:	// unsupported sequence
					return NULL;
			}
			++*buf;
			continue;
		}

		if (**buf == '"') // end of quoted string
		{
			++*buf;
			*write = '\0';
			break;
		}

		*write++ = **buf;
		++*buf;
	}

	return ret;
}

/*
========================
VDB_ParseEntry

Parses either a simple key/value pair or a node
========================
*/
static boolean VDB_ParseEntry (char **buf, vdbcontext_t *ctx)
{
	char *name;

	while (q_isspace (**buf))
		++*buf;
	if (!**buf) // end of buffer
		return true;

	name = VDB_ParseString (buf);
	if (!name)
		return false;

	while (q_isspace (**buf))
		++*buf;

	if (**buf == '"') // key-value pair
	{
		char *value = VDB_ParseString (buf);
		if (!value)
			return false;
		ctx->callback (ctx, name, value);
		return true;
	}

	if (**buf == '{') // node
	{
		++*buf;
		if (ctx->depth == countof (ctx->path))
			return false;
		ctx->path[ctx->depth++] = name;

		while (**buf)
		{
			while (q_isspace (**buf))
				++*buf;

			if (**buf == '}')
			{
				++*buf;
				--ctx->depth;
				break;
			}

			if (**buf == '"')
			{
				if (!VDB_ParseEntry (buf, ctx))
					return false;
				else
					continue;
			}

			if (**buf)
				return false;
		}

		return true;
	}

	return false;
}

/*
========================
VDB_Parse

Parses the given buffer in-place, calling the specified callback for each key/value pair
========================
*/
static boolean VDB_Parse (char *buf, vdbcallback_t callback, void *userdata)
{
	vdbcontext_t ctx;
	ctx.userdata = userdata;
	ctx.callback = callback;
	ctx.depth = 0;

	while (*buf)
		if (!VDB_ParseEntry (&buf, &ctx))
			return false;

	return true;
}

/*
========================
Steam library folder config parsing

Examines all steam library folders and returns the path of the one containing the game with the given appid
========================
*/
typedef struct {
	const char	*appid;
	const char	*current;
	const char	*result;
} libsparser_t;

static void VDB_OnLibFolderProperty (vdbcontext_t *ctx, const char *key, const char *value)
{
	libsparser_t *parser = (libsparser_t *) ctx->userdata;
	int idx;

	if (ctx->depth >= 2 && !strcmp (ctx->path[0], "libraryfolders") && sscanf (ctx->path[1], "%d", &idx) == 1)
	{
		if (ctx->depth == 2)
		{
			if (!strcmp (key, "path"))
				parser->current = value;
		}
		else if (ctx->depth == 3 && !strcmp (key, parser->appid) && !strcmp (ctx->path[2], "apps"))
		{
			parser->result = parser->current;
		}
	}
}

/*
========================
Steam application manifest parsing

Finds the path relative to the library folder
========================
*/
typedef struct {
	const char *result;
} acfparser_t;

static void ACF_OnManifestProperty (vdbcontext_t *ctx, const char *key, const char *value)
{
	acfparser_t *parser = (acfparser_t *) ctx->userdata;
	if (ctx->depth == 1 && !strcmp (key, "installdir") && !strcmp (ctx->path[0], "AppState"))
		parser->result = value;
}

/*
========================
Steam_IsValidPath

Returns true if the given path contains a valid Steam install
(based on the existence of a config/libraryfolders.vdf file)
========================
*/
boolean Steam_IsValidPath (const char *path)
{
	char libpath[STEAM_PATH_MAX_SIZE];
	if ((size_t) snprintf (libpath, sizeof (libpath), "%s/config/libraryfolders.vdf", path) >= sizeof (libpath))
		return false;

    // FIXME: WIN32
    return I_FileExists(libpath);
}

/*
========================
Steam_ReadLibFolders

Returns malloc'ed buffer with Steam library folders config
========================
*/
static char *Steam_ReadLibFolders (void)
{
	char steam_dir[STEAM_PATH_MAX_SIZE];
	if (!Sys_GetSteamDir (steam_dir, STEAM_PATH_MAX_SIZE))
		return NULL;

	char vfd_file[STEAM_PATH_MAX_SIZE];

	if ((size_t) snprintf (vfd_file, STEAM_PATH_MAX_SIZE, "%s/config/libraryfolders.vdf", steam_dir) >= STEAM_PATH_MAX_SIZE)
		return NULL;
	return (char *) COM_LoadMallocFile_TextMode_OSPath (vfd_file, NULL);
}

/*
========================
Steam_FindGame

Finds the Steam library and subdirectory for the given appid
========================
*/
boolean Steam_FindGame (steamgame_t *game, int appid)
{
	char			appidstr[32], path[STEAM_PATH_MAX_SIZE];
	char			*steamcfg, *manifest;
	libsparser_t	libparser;
	acfparser_t		acfparser;
	size_t			liblen, sublen;
	boolean		ret = false;

	game->appid = appid;
	game->subdir = NULL;
	game->library[0] = 0;

	steamcfg = Steam_ReadLibFolders ();
	if (!steamcfg)
	{
		I_Printf ("Steam library not found.\n");
		return false;
	}

	snprintf (appidstr, sizeof (appidstr), "%d", appid);
	memset (&libparser, 0, sizeof (libparser));
	libparser.appid = appidstr;
	if (!VDB_Parse (steamcfg, VDB_OnLibFolderProperty, &libparser))
	{
		I_Printf ("ERROR: Couldn't parse Steam library.\n");
		goto done_cfg;
	}

	if ((size_t) snprintf (path, sizeof (path), "%s/steamapps/appmanifest_%s.acf", libparser.result, appidstr) >= sizeof (path))
	{
		I_Printf ("ERROR: Couldn't read Steam manifest for app %s (path too long).\n", appidstr);
		goto done_cfg;
	}

	manifest = (char *) COM_LoadMallocFile_TextMode_OSPath (path, NULL);
	if (!manifest)
	{
		I_Printf ("ERROR: Couldn't read Steam manifest for app %s.\n", appidstr);
		goto done_cfg;
	}

	memset (&acfparser, 0, sizeof (acfparser));
	if (!VDB_Parse (manifest, ACF_OnManifestProperty, &acfparser))
	{
		I_Printf ("ERROR: Couldn't parse Steam manifest for app %s.\n", appidstr);
		goto done_manifest;
	}

	liblen = strlen (libparser.result);
	sublen = strlen (acfparser.result);

	if (liblen + 1 + sublen + 1 > countof (game->library))
	{
		I_Printf ("ERROR: Path for Steam app %s is too long.\n", appidstr);
		goto done_manifest;
	}

	memcpy (game->library, libparser.result, liblen + 1);
	game->subdir = game->library + liblen + 1;
	memcpy (game->subdir, acfparser.result, sublen + 1);
	ret = true;

done_manifest:
	free (manifest);
done_cfg:
	free (steamcfg);

	return ret;
}

boolean Steam_ResolvePath (char *path, size_t pathsize, const steamgame_t *game)
{
	return
		game->subdir &&
		(size_t) snprintf (path, pathsize, "%s/steamapps/common/%s", game->library, game->subdir) < pathsize
	;
}
