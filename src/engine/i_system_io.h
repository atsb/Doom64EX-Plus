#ifndef _I_SYSTEM_IO_H
#define _I_SYSTEM_IO_H

#include <SDL3/SDL_platform_defines.h>

#ifdef SDL_PLATFORM_WIN32
#include <windows.h> // for MAX_PATH
#include <io.h>
#define F_OK 0
#define W_OK 2
#define R_OK 4
#define open _open
#define read _read
#define write _write
#define close _close
#define access _access
#else
#include <unistd.h>
#include <limits.h> // for PATH_MAX, usually 4096
#define MAX_PATH PATH_MAX
#endif

// Gibbon - hack from curl to deal with some crap
// fixes S_ISREG not available with MSVC
#if defined(S_IFMT)
#if !defined(S_ISREG) && defined(S_IFREG)
#define S_ISREG(m) (((m)&S_IFMT) == S_IFREG)
#endif
#if !defined(S_ISDIR) && defined(S_IFDIR)
#define S_ISDIR(m) (((m)&S_IFMT) == S_IFDIR)
#endif

#endif

typedef char filepath_t[MAX_PATH];

#endif
