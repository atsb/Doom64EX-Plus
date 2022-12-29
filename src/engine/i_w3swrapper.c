// Emacs style mode select   -*- C -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 2022 André Guilherme
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
// 02111-1307, USA.
//
//-----------------------------------------------------------------------------

#include "i_w3swrapper.h"
#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include <time.h>

#ifndef _WIN32
void w3ssleep(dword usecs)
{
	struct timespec tc;
	tc.tv_sec = usecs / 1000;
	tc.tv_nsec = (usecs % 1000) * 1000000;
	nanosleep(&tc, NULL);
}
#endif

#ifndef NO_VSNPRINTF
// On Windows, vsnprintf() is _vsnprintf().
#ifdef _WIN32
#if _MSC_VER < 1400 /* not needed for Visual Studio 2008 */
#define vsnprintf _vsnprintf
#endif
#endif
// Safe, portable vsnprintf().
int M_vsnprintf(char* buf, size_t buf_len, const char* s, va_list args)
{
    int result;

    if (buf_len < 1)
    {
        return 0;
    }

    // Windows (and other OSes?) has a vsnprintf() that doesn't always
    // append a trailing \0. So we must do it, and write into a buffer
    // that is one byte shorter; otherwise this function is unsafe.
    result = vsnprintf(buf, buf_len, s, args);

    // If truncated, change the final char in the buffer to a \0.
    // A negative result indicates a truncated buffer on Windows.
    if (result < 0 || result >= buf_len)
    {
        buf[buf_len - 1] = '\0';
        result = buf_len - 1;
    }

    return result;
}
#endif

//
// dhtoi
//

int htoi(int8_t* str) {
	int8_t* s;
	int num;

	num = 0;
	s = str;

	while (*s) {
		num <<= 4;
		if (*s >= '0' && *s <= '9') {
			num += *s - '0';
		}
		else if (*s >= 'a' && *s <= 'f') {
			num += 10 + *s - 'a';
		}
		else if (*s >= 'A' && *s <= 'F') {
			num += 10 + *s - 'A';
		}
		else {
			return 0;
		}
		s++;
	}

	return num;
}

//
// dfcmp
//

boolean fcmp(float f1, float f2) {
	float precision = 0.00001f;
	if (((f1 - precision) < f2) &&
		((f1 + precision) > f2)) {
		return true;
	}
	else {
		return false;
	}
}
