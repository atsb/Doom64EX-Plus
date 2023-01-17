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

#ifndef __OBJECT_H__
#define __OBJECT_H__

#define BEGIN_CLASS(classname)                          \
class classname {                                       \
public:                                                 \
    static kexRTTI          info;                       \
    static kexObject        *Create(void);              \
    virtual kexRTTI         *GetInfo(void) const
    
#define BEGIN_EXTENDED_CLASS(classname, supername)      \
class classname : public supername {                    \
public:                                                 \
    static kexRTTI          info;                       \
    static kexObject        *Create(void);              \
    virtual kexRTTI         *GetInfo(void) const

#define END_CLASS() }

#define DEFINE_CLASS(classname, supername)                      \
    kexRTTI classname::info(#classname, #supername,             \
        classname::Create,                                      \
        (void(kexObject::*)(void))&classname::Spawn,            \
        (void(kexObject::*)(kexBinFile*))&classname::Save,      \
        (void(kexObject::*)(kexBinFile*))&classname::Load);     \
    kexRTTI *classname::GetInfo(void) const {                   \
        return &(classname::info);                              \
    }

#define DECLARE_CLASS(classname, supername)             \
    DEFINE_CLASS(classname, supername)                  \
    kexObject *classname::Create(void) {                \
        return new classname;                           \
    }

#define DECLARE_ABSTRACT_CLASS(classname, supername)    \
    DEFINE_CLASS(classname, supername)                  \
    kexObject *classname::Create(void) {                \
        return NULL;                                    \
    }

class kexBinFile;
class kexObject;
class kexRTTI;

typedef void(kexObject::*spawnObjFunc_t)(void);
typedef void(kexObject::*saveObjFunc_t)(kexBinFile*);
typedef void(kexObject::*loadObjFunc_t)(kexBinFile*);

class kexRTTI {
public:
                            kexRTTI(const char *classname, const char *supername,
                                kexObject *(*Create)(void),
                                void(kexObject::*Spawn)(void),
                                void(kexObject::*Save)(kexBinFile*),
                                void(kexObject::*Load)(kexBinFile*));
                            ~kexRTTI(void);
                            
    void                    Init(void);
    void                    Destroy(void);
    bool                    InstanceOf(const kexRTTI *objInfo) const;
    kexObject               *(*Create)(void);
    void                    (kexObject::*Spawn)(void);
    void                    (kexObject::*Save)(kexBinFile*);
    void                    (kexObject::*Load)(kexBinFile*);
    
    int                     type_id;
    const char              *classname;
    const char              *supername;
    kexRTTI                 *next;
    kexRTTI                 *super;
};

BEGIN_CLASS(kexObject);
                            ~kexObject(void);
    
    const char              *ClassName(void) const;
    const kexStr            ClassString(void) const;
    const char              *SuperName(void) const;
    const kexStr            SuperString(void) const;
    bool                    InstanceOf(const kexRTTI *objInfo) const;
    void                    CallSpawn(void);
    void                    CallSave(kexBinFile *binFile);
    void                    CallLoad(kexBinFile *binFile);
    void                    Spawn(void);
    void                    Save(kexBinFile *saveFile);
    void                    Load(kexBinFile *LoadFile);
    spawnObjFunc_t          ExecSpawnFunction(kexRTTI *objInfo);
    saveObjFunc_t           ExecSaveFunction(kexRTTI *objInfo, kexBinFile *binFile);
    loadObjFunc_t           ExecLoadFunction(kexRTTI *objInfo, kexBinFile *binFile);

    void                    *operator new(size_t s);
    void                    operator delete(void *ptr);
    
    static void             Init(void);
    static void             Shutdown(void);
    static kexRTTI          *Get(const char *classname);
    static kexObject        *Create(const char *name);
    static void             ListClasses(void);
    
    static int              roverID;
    static kexRTTI          *root;
    
private:
    static bool             bInitialized;
    static int              numObjects;
END_CLASS();

#endif
