#ifndef _LAUNCHER_H_
#define _LAUNCHER_H_

#include <windows.h>
#include <stdint.h>
#include <tchar.h>
#include <stdlib.h>
#include <commctrl.h>
#include <rpcdce.h>
#include <stdio.h>
#include <ctype.h>
#include <stdarg.h>
#include <fcntl.h>
#include <io.h>
#include <math.h>
#include <sys/stat.h>
#include "resource.h"

typedef uint8_t	byte;
typedef uint16_t	word;
typedef uint32_t	uint;
typedef BOOL			bool;
typedef byte* cache;
typedef wchar_t* path[MAX_PATH];

enum { false = 0, true };

void L_Complain(wchar_t* fmt, ...);

#endif
