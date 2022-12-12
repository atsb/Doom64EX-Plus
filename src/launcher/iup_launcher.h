#ifndef IUP_LAUNCHER_H_
#define IUP_LAUNCHER_H_

#include <iup.h>
#include <stdint.h>
#include <stdlib.h>
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
typedef byte* cache;
typedef wchar_t* path[260];

void L_Complain(wchar_t* fmt, ...);

#endif