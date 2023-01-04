// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// Copyright(C) 2012 Samuel Villarreal
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

#ifndef __BLAM_COMMON__
#define __BLAM_COMMON__

void Com_Printf(char* s, ...);
void Com_Error(char *fmt, ...);
void* Com_Alloc(int size);
void Com_Free(void **ptr);
char* Com_BaseDir(void);
int Com_ReadFile(const char* name, byte** buffer);
int Com_ReadBinaryFile(const char* name, byte** buffer);
dboolean Com_SetWriteFile(char const* name, ...);
dboolean Com_SetWriteBinaryFile(char const* name, ...);
void Com_CloseFile(void);
long Com_FileLength(FILE *handle);
int Com_FileExists(const char *filename);
byte *Com_GetFileBuffer(byte *buffer);
byte Com_Read8(byte* buffer);
short Com_Read16(byte* buffer);
int Com_Read32(byte* buffer);
void Com_Write8(byte value);
void Com_Write16(short value);
void Com_Write32(int value);
void Com_FPrintf(char* s, ...);
dboolean Com_HasPath(char *name);
void Com_StripExt(char *name);
void Com_StripPath(char *name);

#endif