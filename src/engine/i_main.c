// Emacs style mode select   -*- C -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 1993-1997 Id Software, Inc.
// Copyright(C) 2007-2012 Samuel Villarreal
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
//
// DESCRIPTION:
//    Main program
//
//-----------------------------------------------------------------------------

#ifndef C89
#include <stdbool.h>
#endif
#include "i_w3swrapper.h"

#ifdef _MSC_VER
#include "i_opndir.h"
#else
#include <dirent.h>
#endif

#include "doomdef.h"
#include "doomstat.h"
#include "d_main.h"

#include <SDL.h>

#include "i_video.h"
#include "m_misc.h"
#include "i_system.h"
#include "con_console.h"

const int8_t version_date[] = __DATE__;

//
// _dprintf
//

void _dprintf(const int8_t* s, ...) {
	static int8_t msg[MAX_MESSAGE_SIZE];
	va_list    va;

	va_start(va, s);
	vsprintf(msg, s, va);
	va_end(va);

	players[consoleplayer].message = msg;
}

//
// dmemcpy
//

void* dmemcpy(void* s1, const void* s2, size_t n) {
	int8_t* r1 = s1;
	const int8_t* r2 = s2;

	while (n) {
		*r1++ = *r2++;
		--n;
	}

	return s1;
}

//
// dmemset
//

void* dmemset(void* s, dword c, size_t n) {
	int8_t* p = (int8_t*)s;

	while (n) {
		*p++ = (int8_t)c;
		--n;
	}

	return s;
}

//
// dstrcpy
//

int8_t* dstrcpy(int8_t* dest, const int8_t* src) {
	dstrncpy(dest, src, dstrlen(src));
	return dest;
}

//
// dstrncpy
//

void dstrncpy(int8_t* dest, const int8_t* src, int maxcount) {
	int8_t* p1 = dest;
	const int8_t* p2 = src;
	while ((maxcount--) >= 0) {
		*p1++ = *p2++;
	}
}

//
// dstrcmp
//

int dstrcmp(const int8_t* s1, const int8_t* s2) {
	while (*s1 && *s2) {
		if (*s1 != *s2) {
			return *s2 - *s1;
		}
		s1++;
		s2++;
	}
	if (*s1 != *s2) {
		return *s2 - *s1;
	}
	return 0;
}

//
// dstrncmp
//

int dstrncmp(const int8_t* s1, const int8_t* s2, int len) {
	while (*s1 && *s2) {
		if (*s1 != *s2) {
			return *s2 - *s1;
		}
		s1++;
		s2++;
		if (!--len) {
			return 0;
		}
	}
	if (*s1 != *s2) {
		return *s2 - *s1;
	}
	return 0;
}

//
// dstricmp
//

int dstricmp(const int8_t* s1, const int8_t* s2) {
	return strcasecmp(s1, s2);
}

//
// dstrnicmp
//

int dstrnicmp(const int8_t* s1, const int8_t* s2, int len) {
	return strncasecmp(s1, s2, len);
}

//
// dstrupr
//

void dstrupr(int8_t* s) {
	int8_t c;

	while ((c = *s) != 0) {
		if (c >= 'a' && c <= 'z') {
			c -= 'a' - 'A';
		}
		*s++ = c;
	}
}

//
// dstrlwr
//

void dstrlwr(int8_t* s) {
	int8_t c;

	while ((c = *s) != 0) {
		if (c >= 'A' && c <= 'Z') {
			c += 32;
		}
		*s++ = c;
	}
}

//
// dstrnlen
//

int dstrlen(const int8_t* string) {
	int rc = 0;
	if (string)
		while (*(string++)) {
			rc++;
		}
	else {
		rc = -1;
	}

	return rc;
}

//
// dstrrchr
//

int8_t* dstrrchr(int8_t* s, int8_t c) {
	int len = dstrlen(s);
	s += len;
	while (len--)
		if (*--s == c) {
			return s;
		}
	return 0;
}

//
// dstrcat
//

void dstrcat(int8_t* dest, const int8_t* src) {
	dest += dstrlen(dest);
	dstrcpy(dest, src);
}

//
// dstrstr
//

int8_t* dstrstr(int8_t* s1, int8_t* s2) {
	int8_t* p = s1;
	int len = dstrlen(s2);

	for (; (p = dstrrchr(p, *s2)) != 0; p++) {
		if (dstrncmp(p, s2, len) == 0) {
			return p;
		}
	}

	return 0;
}

//
// datoi
//

int datoi(const int8_t* str) {
	int val;
	int sign;
	int c;

	if (*str == '-') {
		sign = -1;
		str++;
	}
	else {
		sign = 1;
	}

	val = 0;

	// check for hex

	if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
		str += 2;
		while (1) {
			c = *str++;
			if (c >= '0' && c <= '9') {
				val = (val << 4) + c - '0';
			}
			else if (c >= 'a' && c <= 'f') {
				val = (val << 4) + c - 'a' + 10;
			}
			else if (c >= 'A' && c <= 'F') {
				val = (val << 4) + c - 'A' + 10;
			}
			else {
				return val * sign;
			}
		}
	}

	// check for character

	if (str[0] == '\'') {
		return sign * str[1];
	}

	// assume decimal

	while (1) {
		c = *str++;
		if (c < '0' || c > '9') {
			return val * sign;
		}

		val = val * 10 + c - '0';
	}

	return 0;
}

//
// datof
//

float datof(int8_t* str) {
	double    val;
	int        sign;
	int        c;
	int        decimal, total;

	if (*str == '-') {
		sign = -1;
		str++;
	}
	else {
		sign = 1;
	}

	val = 0;

	// check for hex
	if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
		str += 2;
		while (1) {
			c = *str++;
			if (c >= '0' && c <= '9') {
				val = (val * 16) + c - '0';
			}
			else if (c >= 'a' && c <= 'f') {
				val = (val * 16) + c - 'a' + 10;
			}
			else if (c >= 'A' && c <= 'F') {
				val = (val * 16) + c - 'A' + 10;
			}
			else {
				return (float)val * sign;
			}
		}
	}

	// check for character
	if (str[0] == '\'') {
		return (float)sign * str[1];
	}

	// assume decimal
	decimal = -1;
	total = 0;
	while (1) {
		c = *str++;
		if (c == '.') {
			decimal = total;
			continue;
		}
		if (c < '0' || c > '9') {
			break;
		}
		val = val * 10 + c - '0';
		total++;
	}

	if (decimal == -1) {
		return (float)val * sign;
	}

	while (total > decimal) {
		val /= 10;
		total--;
	}

	return (float)val * sign;
}

//
// dhtoi
//

int dhtoi(int8_t* str) {
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

dboolean dfcmp(float f1, float f2) {
	float precision = 0.00001f;
	if (((f1 - precision) < f2) &&
		((f1 + precision) > f2)) {
		return true;
	}
	else {
		return false;
	}
}

//
// D_abs
//

int D_abs(int x) {
	fixed_t _t = (x), _s;
	_s = _t >> (8 * sizeof _t - 1);
	return (_t ^ _s) - _s;
}

//
// dfabs
//

float D_fabs(float x) {
	return x < 0 ? -x : x;
}

//
// dsprintf
//

int dsprintf(int8_t* buf, const int8_t* format, ...) {
	va_list arg;
	int x;

	va_start(arg, format);
#ifdef HAVE_VSNPRINTF
	x = w3svsnprintf(buf, dstrlen(buf), format, arg);
#else
	x = w3svsnprintf(buf, format, arg);
#endif
	va_end(arg);

	return x;
}

//
// dsnprintf
//

int dsnprintf(int8_t* src, size_t n, const int8_t* str, ...) {
	int x;
	va_list argptr;
	va_start(argptr, str);

#ifdef HAVE_VSNPRINTF
	x = w3svsnprintf(src, n, str, argptr);
#else
	x = w3svsnprintf(src, str, argptr);
#endif

	va_end(argptr);

	return x;
}

//
// main
//

int main(int argc, char *argv[]) {
	myargc = argc;
	myargv = argv;

	D_DoomMain();

	return 0;
}
