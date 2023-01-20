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

#ifndef __RESOURCE_MANAGER_H__
#define __RESOURCE_MANAGER_H__

#include "cachefilelist.h"

template<class type>
class kexResourceManager {
public:
    virtual void            Init(void);
    virtual void            Update(void);
    virtual void            Shutdown(void);

    type                    *Load(const char *file);

    virtual type            *OnLoad(const char *file) = 0;

    kexHashList<type>       dataList;
};

//
// kexResourceManager::Init
//
template<class type>
void kexResourceManager<type>::Init(void) {
}

//
// kexResourceManager::Update
//
template<class type>
void kexResourceManager<type>::Update(void) {
}

//
// kexResourceManager::Shutdown
//
template<class type>
void kexResourceManager<type>::Shutdown(void) {
     for(int i = 0; i < MAX_HASH; i++) {
        for(type *obj = dataList.GetData(i); obj; obj = dataList.Next()) {
            obj->Delete();
        }
     }
}

//
// kexResourceManager::Load
//
template<class type>
type *kexResourceManager<type>::Load(const char *file) {
    type *obj;

    if(file == NULL) {
        return NULL;
    }
    else if(file[0] == 0) {
        return NULL;
    }

    if(!(obj = dataList.Find(file))) {
        obj = OnLoad(file);
    }

    return obj;
}

#endif
