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

#ifndef __BINFILE_H__
#define __BINFILE_H__

#include "kexlib/math/mathlib.h"

class kexBinFile
{
public:
    kexBinFile(void);
    ~kexBinFile(void);

    bool                Open(const char *file, kexHeapBlock &heapBlock = hb_static);
    bool                Create(const char *file);
    void                Close(void);
    bool                Exists(const char *file);
    int                 Length(void);
    void                Duplicate(const char *newFileName);

    byte                Read8(void);
    short               Read16(void);
    int                 Read32(void);
    float               ReadFloat(void);
    kexVec3             ReadVector(void);
    kexStr              ReadString(void);

    void                Write8(const byte val);
    void                Write16(const short val);
    void                Write32(const int val);
    void                WriteFloat(const float val);
    void                WriteVector(const kexVec3 &val);
    void                WriteString(const kexStr &val);

    int                 GetOffsetValue(int id);
    byte                *GetOffset(int id,
                                   byte *subdata = NULL,
                                   int *count = NULL);

    FILE                *Handle(void) const { return handle; }
    byte                *Buffer(void) const { return buffer; }
    void                SetBuffer(byte *ptr) { buffer = ptr; }
    byte                *BufferAt(void) const { return &buffer[bufferOffset]; }
    bool                Opened(void) const { return bOpened; }
    void                SetOffset(const int offset) { bufferOffset = offset; }

private:
    FILE                *handle;
    byte                *buffer;
    unsigned int        bufferOffset;
    bool                bOpened;
};

#endif
