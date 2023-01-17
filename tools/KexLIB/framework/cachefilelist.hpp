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

#ifndef __CACHEFILELIST_H__
#define __CACHEFILELIST_H__

template<class type>
class kexHashList {
public:
    type                *Add(const char *tname, kexHeapBlock &hb = hb_static);
    type                *Find(const char *tname) const;
    type                *GetData(const int index);
    type                *Next(void);
    char                *GetName(const int index);

    typedef struct hashKey_s {
        type            data;
        filepath_t      name;
        hashKey_s       *next;
    } hashKey_t;

    hashKey_t           *hashlist[MAX_HASH];
    hashKey_t           *rover;
};

//
// kexHashList::Add
//
template<class type>
type *kexHashList<type>::Add(const char *tname, kexHeapBlock &hb) {
    unsigned int hash;

    hashKey_t *o = (hashKey_t*)Mem_Calloc(sizeof(hashKey_t), hb);
    strncpy(o->name, tname, MAX_FILEPATH);

    // add to hash for future reference
    hash = kexStr::Hash(o->name);
    o->next = hashlist[hash];
    hashlist[hash] = o;

    return &o->data;
}

//
// kexHashList::Find
//
template<class type>
type *kexHashList<type>::Find(const char *tname) const {
    hashKey_t *t;
    unsigned int hash;

    hash = kexStr::Hash(tname);

    for(t = hashlist[hash]; t; t = t->next) {
        if(!strcmp(tname, t->name)) {
            return &t->data;
        }
    }

    return NULL;
}

//
// kexHashList::GetData
//
template<class type>
type *kexHashList<type>::GetData(const int index) {
    rover = hashlist[index];
    return &rover->data;
}

//
// kexHashList::Next
//
template<class type>
type *kexHashList<type>::Next(void) {
    if(rover->next) {
        rover = rover->next;
        return &rover->data;
    }

    return NULL;
}

//
// kexHashList::GetName
//
template<class type>
char *kexHashList<type>::GetName(const int index) {
    return rover->name;
}

#endif
