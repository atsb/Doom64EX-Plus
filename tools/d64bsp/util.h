//------------------------------------------------------------------------
// UTILITY : general purpose functions
//------------------------------------------------------------------------
//
//  GL-Friendly Node Builder (C) 2000-2007 Andrew Apted
//
//  Based on 'BSP 2.3' by Colin Reed, Lee Killough and others.
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//------------------------------------------------------------------------

#ifndef __GLBSP_UTIL_H__
#define __GLBSP_UTIL_H__

/* ----- useful macros ---------------------------- */
#include <math.h>
//#include <stdio.h>

#ifndef I_ROUND
#define I_ROUND(x)  ((int) (((x) < 0.0f) ? ((x) - 0.5f) : ((x) + 0.5f)))
#endif

/*----- W3S Wrapper ------------------------------------------*/

#ifdef _WIN32
#define w3sfopen fopen_s
#define w3ssetbuf setvbuf
#define w3sstrcat strcat_s
#define w3sstrcpy strcpy_s
#define w3sstrncpy strncpy_s
#define w3ssprintf sprintf_s
#define w3ssnprintf _snprintf_s
#define w3sstrerror strerror_s
#define w3svsnprintf _vsnprintf_s
#else
#define w3sfopen fopen
#define w3sstrcat strcat
#define w3sstrncpy strncpy
#define w3ssprintf sprintf
#define w3sstrerror strerror
#define w3svsnprintf _vsnprintf
#endif

/* ----- function prototypes ---------------------------- */

// allocate and clear some memory.  guaranteed not to fail.
void *UtilCalloc(int size);

// re-allocate some memory.  guaranteed not to fail.
void *UtilRealloc(void *old, int size);

// duplicate a string.
char *UtilStrDup(const char *str);
char *UtilStrNDup(const char *str, int size);

// format the string and return the allocated memory.
// The memory must be freed with UtilFree.
char *UtilFormat(const char *str, ...) GCCATTR((format (printf, 1, 2)));

// free some memory or a string.
void UtilFree(void *data);

// compare two strings case insensitively.
int UtilStrCaseCmp(const char *A, const char *B);

// return an allocated string for the current data and time,
// or NULL if an error occurred.
char *UtilTimeString(void);

// round a positive value up to the nearest power of two.
int UtilRoundPOW2(int x);

// compute angle & distance from (0,0) to (dx,dy)
angle_g UtilComputeAngle(float_g dx, float_g dy);
#define UtilComputeDist(dx,dy)  sqrt((dx) * (dx) + (dy) * (dy))

// compute the parallel and perpendicular distances from a partition
// line to a point.
//
#define UtilParallelDist(part,x,y)  \
    (((x) * (part)->pdx + (y) * (part)->pdy + (part)->p_para)  \
     / (part)->p_length)

#define UtilPerpDist(part,x,y)  \
    (((x) * (part)->pdy - (y) * (part)->pdx + (part)->p_perp)  \
     / (part)->p_length)

// check if the file exists.
int UtilFileExists(const char *filename);

// checksum functions
void Adler32_Begin(uint32_g *crc);
void Adler32_AddBlock(uint32_g *crc, const uint8_g *data, int length);
void Adler32_Finish(uint32_g *crc);

#endif /* __GLBSP_UTIL_H__ */
