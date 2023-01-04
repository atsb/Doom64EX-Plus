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
//
// DESCRIPTION: Binary file operations
//
//-----------------------------------------------------------------------------

#include "kexlib.h"

//
// kexBinFile::kexBinFile
//

kexBinFile::kexBinFile(void) {
    this->handle = NULL;
    this->buffer = NULL;
    this->bufferOffset = 0;
}

//
// kexBinFile::~kexBinFile
//

kexBinFile::~kexBinFile(void) {
    Close();
}

//
// kexBinFile::Open
//

bool kexBinFile::Open(const char *file, kexHeapBlock &heapBlock) {
    int buffsize = kexlib::fileSystem->OpenFile(file, (byte**)(&buffer), heapBlock);

    if(buffsize > 0) {
        bOpened = true;
        bufferOffset = 0;
        return true;
    }

    return false;
}

//
// kexBinFile::OpenStream
//

bool kexBinFile::OpenStream(const char *file) {
    if((handle = fopen(file, "rb"))) {
        bOpened = true;
        return true;
    }

    return false;
}

//
// kexBinFile::Create
//

bool kexBinFile::Create(const char *file) {
    if((handle = fopen(file, "wb"))) {
        bOpened = true;
        bufferOffset = 0;
        return true;
    }

    return false;
}

//
// kexBinFile::Close
//

void kexBinFile::Close(void) {
    if(bOpened == false) {
        return;
    }
    if(handle) {
        fclose(handle);
    }
    if(buffer) {
        Mem_Free(buffer);
    }

    bOpened = false;
}

//
// kexBinFile::Exists
//

bool kexBinFile::Exists(const char *file) {
    FILE *fstream;

    fstream = fopen(file, "r");

    if(fstream != NULL) {
        fclose(fstream);
        return true;
    }
#ifdef KEX_WIN32
    else {
        // If we can't open because the file is a directory, the 
        // "file" exists at least!
        if(errno == 21) {
            return true;
        }
    }
#endif
    return false;
}

//
// kexBinFile::Length
//

int kexBinFile::Length(void) {
    long savedpos;
    long length;

    if(bOpened == false) {
        return 0;
    }

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
// kexBinFile::ReadStream
//

uint kexBinFile::ReadStream(uint offset, byte *buffer, uint length) {
    if(!bOpened || !handle) {
        return 0;
    }

    fseek(handle, offset, SEEK_SET);
    return fread(buffer, 1, length, handle);
}

//
// kexBinFile::Read8
//

byte kexBinFile::Read8(void) {
    byte result;
    result = buffer[bufferOffset++];
    return result;
}

//
// kexBinFile::Read16
//

short kexBinFile::Read16(void) {
    int result;
    result = Read8();
    result |= Read8() << 8;
    return result;
}

//
// kexBinFile::Read32
//

int kexBinFile::Read32(void) {
    int result;
    result = Read8();
    result |= Read8() << 8;
    result |= Read8() << 16;
    result |= Read8() << 24;
    return result;
}

//
// kexBinFile::ReadFloat
//

float kexBinFile::ReadFloat(void) {
    fint_t fi;
    fi.i = Read32();
    return fi.f;
}

//
// kexBinFile::ReadVector2
//

kexVec2 kexBinFile::ReadVector2(void) {
    kexVec2 vec;

    vec.x = ReadFloat();
    vec.z = ReadFloat();

    return vec;
}

//
// kexBinFile::ReadVector3
//

kexVec3 kexBinFile::ReadVector3(void) {
    kexVec3 vec;

    vec.x = ReadFloat();
    vec.y = ReadFloat();
    vec.z = ReadFloat();

    return vec;
}

//
// kexBinFile::ReadVector4
//

kexVec4 kexBinFile::ReadVector4(void) {
    kexVec4 vec;

    vec.x = ReadFloat();
    vec.y = ReadFloat();
    vec.z = ReadFloat();
    vec.w = ReadFloat();

    return vec;
}

//
// kexBinFile::ReadQuaternion
//

kexQuat kexBinFile::ReadQuaternion(void) {
    kexQuat vec;

    vec.x = ReadFloat();
    vec.y = ReadFloat();
    vec.z = ReadFloat();
    vec.w = ReadFloat();

    return vec;
}

//
// kexBinFile::ReadString
//

kexStr kexBinFile::ReadString(void) {
    kexStr str;
    char c = 0;

    while(1) {
        if(!(c = Read8())) {
            break;
        }

        str += c;
    }

    return str;
}

//
// kexBinFile::ReadMatrix
//

kexMatrix kexBinFile::ReadMatrix(void) {
    kexMatrix mtx;

    mtx.vectors[0] = ReadVector4();
    mtx.vectors[1] = ReadVector4();
    mtx.vectors[2] = ReadVector4();
    mtx.vectors[3] = ReadVector4();

    return mtx;
}

//
// kexBinFile::Write8
//

void kexBinFile::Write8(const byte val) {
    fwrite(&val, 1, 1, handle);
    bufferOffset++;
}

//
// kexBinFile::Write16
//

void kexBinFile::Write16(const short val) {
    Write8(val & 0xff);
    Write8((val >> 8) & 0xff);
}

//
// kexBinFile::Write32
//

void kexBinFile::Write32(const int val) {
    Write8(val & 0xff);
    Write8((val >> 8) & 0xff);
    Write8((val >> 16) & 0xff);
    Write8((val >> 24) & 0xff);
}

//
// kexBinFile::WriteFloat
//

void kexBinFile::WriteFloat(const float val) {
    fint_t fi;
    fi.f = val;
    Write32(fi.i);
}

//
// kexBinFile::WriteVector2
//

void kexBinFile::WriteVector2(const kexVec2 &val) {
    WriteFloat(val.x);
    WriteFloat(val.z);
}

//
// kexBinFile::WriteVector3
//

void kexBinFile::WriteVector3(const kexVec3 &val) {
    WriteFloat(val.x);
    WriteFloat(val.y);
    WriteFloat(val.z);
}

//
// kexBinFile::WriteVector4
//

void kexBinFile::WriteVector4(const kexVec4 &val) {
    WriteFloat(val.x);
    WriteFloat(val.y);
    WriteFloat(val.z);
    WriteFloat(val.w);
}

//
// kexBinFile::WriteQuaternion
//

void kexBinFile::WriteQuaternion(const kexQuat &quat) {
    WriteFloat(quat.x);
    WriteFloat(quat.y);
    WriteFloat(quat.z);
    WriteFloat(quat.w);
}

//
// kexBinFile::WriteString
//

void kexBinFile::WriteString(const kexStr &val) {
    const char *c = val.c_str();

    for(int i = 0; i < val.Length(); i++) {
        Write8(c[i]);
    }

    Write8(0);
}

//
// kexBinFile::WriteMatrix
//

void kexBinFile::WriteMatrix(const kexMatrix &mtx) {
    for(int i = 0; i < 4; i++) {
        WriteVector4(mtx.vectors[i]);
    }
}

//
// kexBinFile::GetOffsetValue
//

int kexBinFile::GetOffsetValue(int id) {
    return *(int*)(buffer + (id << 2));
}

//
// kexBinFile::GetOffset
//

byte *kexBinFile::GetOffset(int id, byte *subdata, int *count) {
    byte *data = (subdata == NULL) ? buffer : subdata;

    bufferOffset = GetOffsetValue(id);
    byte *dataOffs = &data[bufferOffset];

    if(count) {
        *count = *(int*)(dataOffs);
    }

    bufferOffset += 4;
    return dataOffs + 4;
}
