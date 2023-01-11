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

#ifndef __KEYMAP_H__
#define __KEYMAP_H__

class kexHashKey {
public:
                                kexHashKey(const char *key, const char *value);
                                kexHashKey(void);
                                ~kexHashKey(void);

    void                        operator=(kexHashKey &hashKey);

    const char                  *GetName(void) { return key.c_str(); }
    const char                  *GetString(void) { return value.c_str(); }

private:
    kexStr                      key;
    kexStr                      value;
};

class kexVec3;

class kexKeyMap {
public:
                                kexKeyMap(void);
                                ~kexKeyMap(void);

    void                        Add(const char *key, const char *value);
    kexHashKey                  *Find(const char *name);
    void                        Empty(void);
    void                        Resize(int newSize);

    const int                   GetHashSize(void) const { return hashSize; }
    kexArray<kexHashKey>        *GetHashList(void) { return hashlist; }
    void                        SetMask(const int mask) { hashMask = mask; }

    bool                        GetFloat(const char *key, float &out, const float defaultValue = 0);
    bool                        GetFloat(const kexStr &key, float &out, const float defaultValue = 0);
    bool                        GetInt(const char *key, int &out, const int defaultValue = 0);
    bool                        GetInt(const kexStr &key, int &out, const int defaultValue = 0);
    bool                        GetBool(const char *key, bool &out, const bool defaultValue = false);
    bool                        GetBool(const kexStr &key, bool &out, const bool defaultValue = false);
    bool                        GetString(const char *key, kexStr &out);
    bool                        GetString(const kexStr &key, kexStr &out);
    bool                        GetVector(const char *key, kexVec3 &out);
    bool                        GetVector(const kexStr &key, kexVec3 &out);

private:
    int                         hashMask;
    int                         hashSize;
    kexArray<kexHashKey>        *hashlist;
};

#endif
