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
// DESCRIPTION: RTTI Object Class
//
//-----------------------------------------------------------------------------

#include "kexlib.h"

//
// kexRTTI::kexRTTI
//

kexRTTI::kexRTTI(const char *classname, const char *supername,
    kexObject *(*Create)(void), void(kexObject::*Spawn)(void),
    void(kexObject::*Save)(kexBinFile*), void(kexObject::*Load)(kexBinFile*)) {
        this->classname     = classname;
        this->supername     = supername;
        this->Create        = Create;
        this->Spawn         = Spawn;
        this->Save          = Save;
        this->Load          = Load;
        this->type_id       = ++kexObject::roverID;
        this->super         = kexObject::Get(supername);

        // link all classes with supers to this class if not referenced yet
        for(kexRTTI *rtti = kexObject::root; rtti != NULL; rtti = rtti->next) {
            if(rtti->super == NULL && !strcmp(rtti->supername, this->classname) &&
                strcmp(rtti->classname, "kexObject")) {
                    rtti->super = this;
            }
        }
    
        if(kexObject::root == NULL) {
            this->next = NULL;
            kexObject::root = this;
        }
        else {
            this->next = kexObject::root;
            kexObject::root = this;
        }
}

//
// kexRTTI::~kexRTTI
//

kexRTTI::~kexRTTI(void) {
    Destroy();
}

//
// kexRTTI::Init
//

void kexRTTI::Init(void) {
}

//
// kexRTTI::Destroy
//

void kexRTTI::Destroy(void) {
}

//
// kexRTTI::InstanceOf
//

bool kexRTTI::InstanceOf(const kexRTTI *objInfo) const {
    return type_id == objInfo->type_id;
}

DECLARE_ABSTRACT_CLASS(kexObject, NULL)

kexRTTI *kexObject::root = NULL;
bool kexObject::bInitialized = false;
int kexObject::roverID = 0;

//
// kexObject::~kexObject
//

kexObject::~kexObject(void) {
}

//
// kexObject::Create
//

kexObject *kexObject::Create(const char *name) {
    const kexRTTI *info;

    if(name == NULL || !(info = kexObject::Get(name))) {
        return NULL;
    }
    
    return info->Create();
}

//
// kexObject::Init
//

void kexObject::Init(void) {
    for(kexRTTI *oi = kexObject::root; oi != NULL; oi = oi->next) {
        oi->Init();
    }
    
    bInitialized = true;
    kexlib::commands->Add("listRuntimeClasses", kexObject::ListClasses);
    kexlib::system->Printf("Runtime Object Initialized\n");
}

//
// kexObject::Shutdown
//

void kexObject::Shutdown(void) {
    for(kexRTTI *oi= kexObject::root; oi != NULL; oi = oi->next) {
        oi->Destroy();
    }
    
    bInitialized = false;
}

//
// kexObject::ExecSpawnFunction
//

spawnObjFunc_t kexObject::ExecSpawnFunction(kexRTTI *objInfo) {
    spawnObjFunc_t func;

    if(objInfo->super) {
        if((func = ExecSpawnFunction(objInfo->super)) == objInfo->Spawn) {
            return func;
        }
    }

    func = objInfo->Spawn;
    (this->*objInfo->Spawn)();

    return func;
}

//
// kexObject::CallSpawn
//

void kexObject::CallSpawn(void) {
    ExecSpawnFunction(GetInfo());
}

//
// kexObject::Spawn
//

void kexObject::Spawn(void) {
}

//
// kexObject::ExecSaveFunction
//

saveObjFunc_t kexObject::ExecSaveFunction(kexRTTI *objInfo, kexBinFile *binFile) {
    saveObjFunc_t func;
    
    if(objInfo->super) {
        if((func = ExecSaveFunction(objInfo->super, binFile)) == objInfo->Save) {
            return func;
        }
    }
    
    func = objInfo->Save;
    (this->*objInfo->Save)(binFile);
    
    return func;
}

//
// kexObject::CallSave
//

void kexObject::CallSave(kexBinFile *binFile) {
    ExecSaveFunction(GetInfo(), binFile);
}

//
// kexObject::Save
//

void kexObject::Save(kexBinFile *saveFile) {
}

//
// kexObject::ExecLoadFunction
//

loadObjFunc_t kexObject::ExecLoadFunction(kexRTTI *objInfo, kexBinFile *binFile) {
    loadObjFunc_t func;
    
    if(objInfo->super) {
        if((func = ExecLoadFunction(objInfo->super, binFile)) == objInfo->Load) {
            return func;
        }
    }
    
    func = objInfo->Load;
    (this->*objInfo->Load)(binFile);
    
    return func;
}

//
// kexObject::CallLoad
//

void kexObject::CallLoad(kexBinFile *binFile) {
    ExecLoadFunction(GetInfo(), binFile);
}

//
// kexObject::Load
//

void kexObject::Load(kexBinFile *loadFile) {
}

//
// kexObject::operator new
//

void *kexObject::operator new(size_t s) {
    return Mem_Calloc(s, hb_object);
}

//
// kexObject::operator delete
//

void kexObject::operator delete(void *ptr) {
    Mem_Free(ptr);
}

//
// kexObject::Get
//

kexRTTI *kexObject::Get(const char *classname) {
    if(classname == NULL) {
        return NULL;
    }

    for(kexRTTI *oi = kexObject::root; oi != NULL; oi = oi->next) {
        if(!strcmp(oi->classname, classname)) {
            return oi;
        }
    }
    return NULL;
}

//
// kexObject::InstanceOf
//

bool kexObject::InstanceOf(const kexRTTI *objInfo) const {
    for(const kexRTTI *oi = GetInfo(); oi; oi = oi->super) {
        if(oi->type_id == objInfo->type_id) {
            return true;
        }
    }

    return false;
}

//
// kexObject::ClassName
//

const char *kexObject::ClassName(void) const {
    return GetInfo()->classname;
}

//
// kexObject::ClassString
//

const kexStr kexObject::ClassString(void) const {
    return kexStr(ClassName());
}

//
// kexObject::SuperName
//

const char *kexObject::SuperName(void) const {
    return GetInfo()->supername;
}

//
// kexObject::SuperString
//

const kexStr kexObject::SuperString(void) const {
    return kexStr(SuperName());
}

//
// kexObject::ListClasses
//

void kexObject::ListClasses(void) {
    kexlib::system->CPrintf(COLOR_GREEN, "-------------- Runtime Classes ---------------\n");
    for(kexRTTI *oi= kexObject::root; oi != NULL; oi = oi->next) {
        kexlib::system->Printf("%s %s %i\n", oi->classname, oi->supername, oi->type_id);
    }
    kexlib::system->CPrintf(COLOR_GREEN, "----------------------------------------------\n\n");
}
