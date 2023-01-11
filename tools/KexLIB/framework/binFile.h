// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// Copyright(C) 2013 Samuel Villarreal
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

#ifndef __BINFILE_H__
#define __BINFILE_H__

class kexVec2;
class kexVec3;
class kexVec4;
class kexQuat;
class kexMatrix;

class kexBinFile {
public:
                        kexBinFile(void);
                        ~kexBinFile(void);

    bool                Open(const char *file, kexHeapBlock &heapBlock = hb_static);
    bool                OpenStream(const char *file);
    bool                Create(const char *file);
    void                Close(void);
    bool                Exists(const char *file);
    int                 Length(void);

    uint                ReadStream(uint offset, byte *buffer, uint length);

    byte                Read8(void);
    short               Read16(void);
    int                 Read32(void);
    float               ReadFloat(void);
    kexVec2             ReadVector2(void);
    kexVec3             ReadVector3(void);
    kexVec4             ReadVector4(void);
    kexQuat             ReadQuaternion(void);
    kexMatrix           ReadMatrix(void);
    kexStr              ReadString(void);

    void                Write8(const byte val);
    void                Write16(const short val);
    void                Write32(const int val);
    void                WriteFloat(const float val);
    void                WriteVector2(const kexVec2 &val);
    void                WriteVector3(const kexVec3 &val);
    void                WriteVector4(const kexVec4 &val);
    void                WriteQuaternion(const kexQuat &quat);
    void                WriteString(const kexStr &val);
    void                WriteMatrix(const kexMatrix &mtx);

    int                 GetOffsetValue(int id);
    byte                *GetOffset(int id,
                                   byte *subdata = NULL,
                                   int *count = NULL);

    FILE                *Handle(void) const { return handle; }
    byte                *Buffer(void) const { return buffer; }
    byte                *BufferAt(void) const { return &buffer[bufferOffset]; }
    const bool          IsOpened(void) const { return bOpened; }

private:
    FILE                *handle;
    byte                *buffer;
    unsigned int        bufferOffset;
    bool                bOpened;
};

#endif
