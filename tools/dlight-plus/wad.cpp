//
// Copyright (c) 2013-2014 Samuel Villarreal
// svkaiser@gmail.com
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
//    1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
//
//   2. Altered source versions must be plainly marked as such, and must not be
//   misrepresented as being the original software.
//
//    3. This notice may not be removed or altered from any source
//    distribution.
//
//-----------------------------------------------------------------------------
//
// DESCRIPTION: General wad loading mechanics
//
//-----------------------------------------------------------------------------

#include "common.h"
#include "wad.h"

//
// kexWadFile::~kexWadFile
//

kexWadFile::~kexWadFile(void)
{
    Close();
}

//
// kexWadFile::Open
//

bool kexWadFile::Open(const char *fileName)
{
    if(!file.Open(fileName))
    {
        return false;
    }

    memcpy(&header, file.Buffer(), sizeof(wadHeader_t));
    lumps = (lump_t*)file.GetOffset(2);
    wadName = fileName;
    bWriting = false;
    return true;
}

//
// kexWadFile::Write
//

void kexWadFile::Write(const char *fileName)
{
    assert(bWriting == true);

    file.Create(fileName);
    file.Write8(header.id[0]);
    file.Write8(header.id[1]);
    file.Write8(header.id[2]);
    file.Write8(header.id[3]);
    file.Write32(header.lmpcount);
    file.Write32(header.lmpdirpos);

    for(unsigned int i = 0; i < writeLumpList.Length(); i++)
    {
        byte *data = writeDataList[i];

        if(!data)
        {
            continue;
        }

        for(int j = 0; j < writeLumpList[i].size; j++)
        {
            file.Write8(data[j]);
        }
    }

    for(unsigned int i = 0; i < writeLumpList.Length(); i++)
    {
        file.Write32(writeLumpList[i].filepos);
        file.Write32(writeLumpList[i].size);
        for(int j = 0; j < 8; j++)
        {
            file.Write8(writeLumpList[i].name[j]);
        }
    }
}

//
// kexWadFile::Close
//

void kexWadFile::Close(void)
{
    if(file.Opened())
    {
        file.Close();
    }
}

//
// kexWadFile::GetLumpFromName
//

lump_t *kexWadFile::GetLumpFromName(const char *name)
{
    char n[9];

    for(int i = 0; i < header.lmpcount; i++)
    {
        // could be optimized here but I really don't feel like
        // wasting time on this
        strncpy(n, lumps[i].name, 8);
        n[8] = 0;

        if(!strncmp(n, name, 8))
        {
            return &lumps[i];
        }
    }

    return NULL;
}

//
// kexWadFile::GetLumpData
//

byte *kexWadFile::GetLumpData(const lump_t *lump)
{
    file.SetOffset(lump->filepos);
    return &file.Buffer()[lump->filepos];
}

//
// kexWadFile::GetLumpData
//

byte *kexWadFile::GetLumpData(const char *name)
{
    lump_t *lump = GetLumpFromName(name);

    if(!lump)
    {
        return NULL;
    }

    file.SetOffset(lump->filepos);
    return &file.Buffer()[lump->filepos];
}

//
// kexWadFile::SetCurrentMap
//

void kexWadFile::SetCurrentMap(const int map)
{
    lump_t *lump = GetLumpFromName(Va("MAP%02d", map));

    if(lump == NULL)
    {
        Error("kexWadFile::SetCurrentMap: MAP%02d not found\n", map);
        return;
    }

    mapLumpID = lump - lumps;

    lump = GetLumpFromName(Va("GL_MAP%02d", map));

    if(lump == NULL)
    {
        Error("kexWadFile::SetCurrentMap: GL_MAP%02d not found\n", map);
        return;
    }

    glMapLumpID = lump - lumps;
    currentmap = map;
}

//
// kexWadFile::GetMapLump
//

lump_t *kexWadFile::GetMapLump(mapLumps_t lumpID)
{
    if(mapLumpID + lumpID >= header.lmpcount)
    {
        return NULL;
    }

    return &lumps[mapLumpID + lumpID];
}

//
// kexWadFile::GetGLMapLump
//

lump_t *kexWadFile::GetGLMapLump(glMapLumps_t lumpID)
{
    if(glMapLumpID + lumpID >= header.lmpcount)
    {
        return NULL;
    }

    return &lumps[glMapLumpID + lumpID];
}

//
// kexWadFile::InitForWrite
//

void kexWadFile::InitForWrite(void)
{
    header.id[0] = 'P';
    header.id[1] = 'W';
    header.id[2] = 'A';
    header.id[3] = 'D';

    writePos = sizeof(wadHeader_t);
    bWriting = true;

    header.lmpcount = 0;
    header.lmpdirpos = writePos;
}

//
// kexWadFile::CopyLumpsFromWadFile
//

void kexWadFile::CopyLumpsFromWadFile(kexWadFile &wadFile)
{
    lump_t lump;

    assert(bWriting == true);

    for(int i = 0; i < wadFile.header.lmpcount; ++i)
    {
        lump = wadFile.lumps[i];
        AddLump(wadFile.lumps[i].name, lump.size, &wadFile.file.Buffer()[lump.filepos]);
    }
}

//
// kexWadFile::CopyLumpsFromWadFile
//

void kexWadFile::CopyLumpsFromWadFile(kexWadFile &wadFile, kexArray<int> &lumpIgnoreList)
{
    lump_t lump;
    unsigned int j;
    bool bSkip;

    assert(bWriting == true);

    for(int i = 0; i < wadFile.header.lmpcount; ++i)
    {
        lump = wadFile.lumps[i];
        bSkip = false;

        for(j = 0; j < lumpIgnoreList.Length(); ++j)
        {
            if(lumpIgnoreList[j] == i)
            {
                bSkip = true;
                break;
            }
        }

        if(bSkip)
        {
            continue;
        }

        AddLump(wadFile.lumps[i].name, lump.size, &wadFile.file.Buffer()[lump.filepos]);
    }
}

//
// kexWadFile::AddLump
//

void kexWadFile::AddLump(const char *name, const int size, byte *data)
{
    lump_t lump;

    assert(bWriting == true);

    lump.filepos = writePos;
    lump.size = size;
    strncpy(lump.name, name, 8);

    writeLumpList.Push(lump);
    writeDataList.Push(data);

    header.lmpcount++;
    header.lmpdirpos += lump.size;
    writePos += lump.size;
}

//
// kexWadFile::CreateBackup
//

void kexWadFile::CreateBackup(void)
{
    kexStr backupName = wadName;

    backupName.StripExtension();
    file.Duplicate(backupName + ".wad.bak");
}
