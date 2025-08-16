#ifndef _I_SYSTEM_IO_H
#define _I_SYSTEM_IO_H

#include <SDL3/SDL_platform_defines.h>

#ifdef SDL_PLATFORM_WINDOWS
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
#endif

#endif