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

#include "types.h"
#include "common.h"
#include "parse.h"

static unsigned long com_fileoffset = 0;
static FILE *com_file = NULL;

//
// Com_Printf
//

void Com_Printf(char* s, ...)
{
    char msg[1024];
    va_list v;
    
    va_start(v,s);
    vsprintf(msg,s,v);
    va_end(v);
    
    printf("%s\n", msg);
}

//
// Com_Error
//

void Com_Error(char *fmt, ...)
{
    va_list	va;
    char buff[1024];
    
    va_start(va, fmt);
    vsprintf(buff, fmt, va);
    va_end(va);
    Com_Printf("\n**************************\n");
    Com_Printf("ERROR: %s\n", buff);
    Com_Printf("**************************\n");

    Com_SetWriteFile("blam_err.txt");
    Com_FPrintf(buff);
    Com_CloseFile();

    exit(1);
}

//
// Com_Alloc
//

void* Com_Alloc(int size)
{
    void *ret = calloc(1, size);
    if(!ret)
        Com_Error("Com_Alloc: Out of memory");

    return ret; 
}

//
// Com_Free
//

void Com_Free(void **ptr)
{
    if(!*ptr)
        Com_Error("Com_Free: Tried to free NULL");

    free(*ptr);
    *ptr = NULL;
}

//
// Com_BaseDir
//

char* Com_BaseDir(void)
{
    static char current_dir_dummy[] = {"./"};
    static char *base;

    if(!base)    // cache multiple requests
    {
        size_t len = strlen(*myargv);
        char *p = (base = malloc(len+1)) + len - 1;

        strcpy(base, *myargv);

        while(p > base && *p!='/' && *p!='\\') *p--=0;

        if(*p=='/' || *p=='\\') *p--=0;

        if(strlen(base)<2)
        {
            free(base);
            base = malloc(1024);
            if(!_getcwd(base,1024))
                strcpy(base, current_dir_dummy);
        }
    }

    return base;
}

//
// va
//

static char *va(char *str, ...)
{
    va_list v;
    static char vastr[1024];

    va_start(v, str);
    vsprintf(vastr, str,v);
    va_end(v);

    return vastr;
}

//
// Com_ReadFile
//

int Com_ReadFile(const char* name, byte** buffer)
{
    FILE *fp = NULL;
    char *fileName;

    errno = 0;

    fileName = va("%s", name);

    if((fp = fopen(fileName, "r")))
    {
        size_t length;

        fseek(fp, 0, SEEK_END);
        length = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        *buffer = Com_Alloc(length);
      
        if(fread(*buffer, 1, length, fp) == length)
        {
            fclose(fp);
            return length;
        }
        
        fclose(fp);
   }

    Com_Error("Com_ReadFile: Couldn't find %s", fileName);

    return -1;
}

//
// Com_ReadFile
//

int Com_ReadBinaryFile(const char* name, byte** buffer)
{
    FILE *fp = NULL;
    char *fileName;

    errno = 0;

    fileName = va("%s", name);

    if((fp = fopen(fileName, "rb")))
    {
        size_t length;

        fseek(fp, 0, SEEK_END);
        length = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        *buffer = Com_Alloc(length);
      
        if(fread(*buffer, 1, length, fp) == length)
        {
            fclose(fp);
            return length;
        }
        
        fclose(fp);
    }

    Com_Error("Com_ReadBinaryFile: Couldn't find %s", fileName);

    return -1;
}

//
// Com_SetWriteFile
//

dboolean Com_SetWriteFile(char const* name, ...)
{
    char filename[1024];
    char *otherFile;
    va_list v;

    va_start(v, name);
    vsprintf(filename, name, v);
    va_end(v);

    errno = 0;

    otherFile = va("%s", filename);

    if(!(com_file = fopen(otherFile, "w")))
    {
        Com_Error("Com_SetWriteFile: Couldn't write %s", otherFile);
        return 0;
    }

    return 1;
}

//
// Com_SetWriteFile
//

dboolean Com_SetWriteBinaryFile(char const* name, ...)
{
    char filename[1024];
    char *otherFile;
    va_list v;

    va_start(v, name);
    vsprintf(filename, name, v);
    va_end(v);

    errno = 0;

    otherFile = va("%s", filename);

    if(!(com_file = fopen(otherFile, "wb")))
    {
        Com_Error("Com_SetWriteBinaryFile: Couldn't write %s", otherFile);
        return 0;
    }

    return 1;
}

//
// Com_CloseFile
//

void Com_CloseFile(void)
{
    fclose(com_file);
}

//
// Com_GetFileBuffer
//

byte *Com_GetFileBuffer(byte *buffer)
{
    return buffer + com_fileoffset;
}

//
// Com_Read8
//

byte Com_Read8(byte* buffer)
{
    byte result;

    result = buffer[com_fileoffset++];

    return result;
}

//
// Com_Read16
//

short Com_Read16(byte* buffer)
{
    int result;

    result = Com_Read8(buffer);
    result |= Com_Read8(buffer) << 8;

    return result;
}

//
// Com_Read32
//

int Com_Read32(byte* buffer)
{
    int result;

    result = Com_Read8(buffer);
    result |= Com_Read8(buffer) << 8;
    result |= Com_Read8(buffer) << 16;
    result |= Com_Read8(buffer) << 24;

    return result;
}

//
// Com_Write8
//

void Com_Write8(byte value)
{
    fwrite(&value, 1, 1, com_file);
    com_fileoffset++;
}

//
// Com_Write16
//

void Com_Write16(short value)
{
    Com_Write8(value & 0xff);
    Com_Write8((value >> 8) & 0xff);
}

//
// Com_Write32
//

void Com_Write32(int value)
{
    Com_Write8(value & 0xff);
    Com_Write8((value >> 8) & 0xff);
    Com_Write8((value >> 16) & 0xff);
    Com_Write8((value >> 24) & 0xff);
}

//
// Com_FPrintf
//

void Com_FPrintf(char* s, ...)
{
    char msg[1024];
    va_list v;
    
    va_start(v,s);
    vsprintf(msg,s,v);
    va_end(v);

    fprintf(com_file, msg);
}

//
// Com_FileLength
//

long Com_FileLength(FILE *handle)
{ 
    long savedpos;
    long length;

    // save the current position in the file
    savedpos = ftell(handle);
    
    // jump to the end and find the length
    fseek(handle, 0, SEEK_END);
    length = ftell(handle);

    // go back to the old location
    fseek(handle, savedpos, SEEK_SET);

    return length;
}

//
// Com_FileExists
// Check if a wad file exists
//

int Com_FileExists(const char *filename)
{
    FILE *fstream;

    fstream = fopen(filename, "r");

    if (fstream != NULL)
    {
        fclose(fstream);
        return 1;
    }
    else
    {
        // If we can't open because the file is a directory, the 
        // "file" exists at least!

        if(errno == 21)
            return 2;
    }

    return 0;
}

#define ASCII_SLASH		47
#define ASCII_BACKSLASH 92

//
// Com_HasPath
//

dboolean Com_HasPath(char *name)
{
    unsigned int i;

    for(i = 0; i < strlen(name); i++)
    {
        if(name[i] == ASCII_BACKSLASH || name[i] == ASCII_SLASH)
            return true;
    }

    return false;
}

//
// Com_StripExt
//

void Com_StripExt(char *name)
{
    char *search;

    search = name + strlen(name) - 1;

    while(*search != ASCII_BACKSLASH && *search != ASCII_SLASH
        && search != name)
    {
        if(*search == '.')
        {
            *search = '\0';
            return;
        }

        search--;
    }
}

void Com_StripPath(char *name)
{
    char *search;
    int len = 0;
    int pos = 0;
    int i = 0;

    len = strlen(name) - 1;
    pos = len + 1;

    for(search = name + len;
        *search != ASCII_BACKSLASH && *search != ASCII_SLASH; search--, pos--)
    {
        if(search == name)
        {
            return;
        }
    }

    if(pos <= 0)
    {
        return;
    }

    for(i = 0; pos < len+1; pos++, i++)
    {
        name[i] = name[pos];
    }

    name[i] = '\0';
}
