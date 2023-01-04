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
//
// DESCRIPTION: Hashkey lookups
//
//-----------------------------------------------------------------------------

#include "kexlib.h"

//
// kexHashKey::kexHashKey
//

kexHashKey::kexHashKey(const char *key, const char *value) {
    this->key = key;
    this->value = value;
}

//
// kexHashKey::kexHashKey
//

kexHashKey::kexHashKey(void) {
    this->key = "";
    this->value = "";
}

//
// kexHashKey::~kexHashKey
//

kexHashKey::~kexHashKey(void) {
}

//
// kexHashKey::operator=
//

void kexHashKey::operator=(kexHashKey &hashKey) {
    this->key = hashKey.key;
    this->value = hashKey.value;
}

//
// kexKeyMap::kexKeyMap
//

kexKeyMap::kexKeyMap(void) {
    this->hashMask = MAX_HASH;
    this->hashSize = 0;
    this->hashlist = NULL;
}

//
// kexKeyMap::~kexKeyMap
//

kexKeyMap::~kexKeyMap(void) {
    if(hashlist != NULL && hashSize != 0) {
        for(int i = 0; i < hashSize; i++) {
            hashlist[i].Empty();
        }
        
        delete[] hashlist;
    }
}

//
// kexKeyMap::Add
//

void kexKeyMap::Add(const char *key, const char *value) {
    int hash = kexStr::Hash(key) & (hashMask-1);

    Resize(hash+1);
    hashlist[hash].Push(kexHashKey(key, value));
}

//
// kexKeyMap::Empty
//

void kexKeyMap::Empty(void) {
    if(hashlist != NULL && hashSize != 0) {
        for(int i = 0; i < hashSize; i++) {
            hashlist[i].Empty();
        }
    }
}

//
// kexKeyMap::Resize
//

void kexKeyMap::Resize(int newSize) {
    kexArray<kexHashKey> *oldList;
    int i;
    
    if(newSize == hashSize) {
        return;
    }

    if(hashlist == NULL && hashSize == 0) {
        hashlist = new kexArray<kexHashKey>[newSize];
        hashSize = newSize;
        return;
    }

    if(newSize < hashSize || hashSize <= 0) {
        return;
    }

    oldList = hashlist;
    hashlist = new kexArray<kexHashKey>[newSize];

    for(i = 0; i < hashSize; i++) {
        hashlist[i] = oldList[i];
    }
    
    oldList->Empty();
    delete[] oldList;
    
    hashSize = newSize;
}

//
// kexKeyMap::Find
//

kexHashKey *kexKeyMap::Find(const char *name) {
    kexHashKey *k;
    kexArray<kexHashKey> *keyList;
    int hash = kexStr::Hash(name) & (hashMask-1);

    if(hash >= hashSize) {
        return NULL;
    }

    keyList = &hashlist[hash];

    for(unsigned int i = 0; i < keyList->Length(); i++) {
        k = &hashlist[hash][i];
        if(!strcmp(name, k->GetName())) {
            return k;
        }
    }
    return NULL;
}

//
// kexKeyMap::GetFloat
//

bool kexKeyMap::GetFloat(const char *key, float &out, const float defaultValue) {
    kexHashKey *k;

    out = defaultValue;

    if(!(k = Find(key))) {
        return false;
    }

    out = (float)atof(k->GetString());
    return true;
}

//
// kexKeyMap::GetFloat
//

bool kexKeyMap::GetFloat(const kexStr &key, float &out, const float defaultValue) {
    return GetFloat(key.c_str(), out, defaultValue);
}

//
// kexKeyMap::GetInt
//

bool kexKeyMap::GetInt(const char *key, int &out, const int defaultValue) {
    kexHashKey *k;

    out = defaultValue;

    if(!(k = Find(key))) {
        return false;
    }

    out = atoi(k->GetString());
    return true;
}

//
// kexKeyMap::GetInt
//

bool kexKeyMap::GetInt(const kexStr &key, int &out, const int defaultValue) {
    return GetInt(key.c_str(), out, defaultValue);
}

//
// kexKeyMap::GetBool
//

bool kexKeyMap::GetBool(const char *key, bool &out, const bool defaultValue) {
    kexHashKey *k;

    out = defaultValue;

    if(!(k = Find(key))) {
        return false;
    }

    out = (atoi(k->GetString()) != 0);
    return true;
}

//
// kexKeyMap::GetBool
//

bool kexKeyMap::GetBool(const kexStr &key, bool &out, const bool defaultValue) {
    return GetBool(key.c_str(), out, defaultValue);
}

//
// kexKeyMap::GetString
//

bool kexKeyMap::GetString(const char *key, kexStr &out) {
    kexHashKey *k;

    if(!(k = Find(key))) {
        return false;
    }

    out = k->GetString();
    return true;
}

//
// kexKeyMap::GetString
//

bool kexKeyMap::GetString(const kexStr &key, kexStr &out) {
    return GetString(key.c_str(), out);
}

//
// kexKeyMap::GetVector
//

bool kexKeyMap::GetVector(const char *key, kexVec3 &out) {
    kexHashKey *k;

    out.Clear();

    if(!(k = Find(key))) {
        return false;
    }

    sscanf(k->GetString(), "%f %f %f", &out.x, &out.y, &out.z);
    return true;
}

//
// kexKeyMap::GetVector
//

bool kexKeyMap::GetVector(const kexStr &key, kexVec3 &out) {
    return GetVector(key.c_str(), out);
}
