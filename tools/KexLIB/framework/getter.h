// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// Copyright(C) 2007-2012 Samuel Villarreal
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

#ifndef __GETTER_H__
#define __GETTER_H__

typedef enum {
    GRT_FLOAT           = 0,
    GRT_INT,
    GRT_BOOL,
    GRT_VEC2,
    GRT_VEC3,
    GRT_VEC4,
    GRT_MAT4,
    GRT_NUMTYPES
} getterReturnType_t;

class kexGetterBase {
public:
    virtual float               GetFloatValue(void) const = 0;
    virtual int                 GetIntValue(void) const = 0;
    virtual bool                GetBoolValue(void) const = 0;
    virtual kexVec2             &GetVec2Value(void) = 0;
    virtual kexVec3             &GetVec3Value(void) = 0;
    virtual kexVec4             &GetVec4Value(void) = 0;
    virtual kexMatrix           &GetMat4Value(void) = 0;
    
    kexStr                      &Name(void) { return name; }
    const getterReturnType_t    ReturnType(void) const { return retType; }
    
protected:
    kexStr                      name;
    getterReturnType_t          retType;
};

template<typename type>
class kexGetter : public kexGetterBase {
public:
    typedef float               (type::*callbackFloat_t)(void);
    typedef int                 (type::*callbackInt_t)(void);
    typedef bool                (type::*callbackBool_t)(void);
    typedef kexVec2             &(type::*callbackVec2_t)(void);
    typedef kexVec3             &(type::*callbackVec3_t)(void);
    typedef kexVec4             &(type::*callbackVec4_t)(void);
    typedef kexMatrix           &(type::*callbackMat4_t)(void);
    
protected:
    type                        *obj;
    callbackFloat_t             callbackFloat;
    callbackInt_t               callbackInt;
    callbackBool_t              callbackBool;
    callbackVec2_t              callbackVec2;
    callbackVec3_t              callbackVec3;
    callbackVec4_t              callbackVec4;
    callbackMat4_t              callbackMat4;
    
public:
    kexGetter(const char *pName, type *pObj, callbackFloat_t pCallback) {
        this->name = pName;
        this->obj = pObj;
        this->retType = GRT_FLOAT;
        this->callbackFloat = pCallback;
    }
    
    kexGetter(const char *pName, type *pObj, callbackInt_t pCallback) {
        this->name = pName;
        this->obj = pObj;
        this->retType = GRT_INT;
        this->callbackInt = pCallback;
    }
    
    kexGetter(const char *pName, type *pObj, callbackBool_t pCallback) {
        this->name = pName;
        this->obj = pObj;
        this->retType = GRT_BOOL;
        this->callbackBool = pCallback;
    }
    
    kexGetter(const char *pName, type *pObj, callbackVec2_t pCallback) {
        this->name = pName;
        this->obj = pObj;
        this->retType = GRT_VEC2;
        this->callbackVec2 = pCallback;
    }
    
    kexGetter(const char *pName, type *pObj, callbackVec3_t pCallback) {
        this->name = pName;
        this->obj = pObj;
        this->retType = GRT_VEC3;
        this->callbackVec3 = pCallback;
    }
    
    kexGetter(const char *pName, type *pObj, callbackVec4_t pCallback) {
        this->name = pName;
        this->obj = pObj;
        this->retType = GRT_VEC4;
        this->callbackVec4 = pCallback;
    }

    kexGetter(const char *pName, type *pObj, callbackMat4_t pCallback) {
        this->name = pName;
        this->obj = pObj;
        this->retType = GRT_MAT4;
        this->callbackMat4 = pCallback;
    }
    
    virtual float               GetFloatValue(void) const { return (obj->*callbackFloat)(); }
    virtual int                 GetIntValue(void) const { return (obj->*callbackInt)(); }
    virtual bool                GetBoolValue(void) const { return (obj->*callbackBool)(); }
    virtual kexVec2             &GetVec2Value(void) { return (obj->*callbackVec2)(); }
    virtual kexVec3             &GetVec3Value(void) { return (obj->*callbackVec3)(); }
    virtual kexVec4             &GetVec4Value(void) { return (obj->*callbackVec4)(); }
    virtual kexMatrix           &GetMat4Value(void) { return (obj->*callbackMat4)(); }
};

#define GETTER_FLOAT(t) typename kexGetter<t>::callbackFloat_t
#define GETTER_INT(t)   typename kexGetter<t>::callbackInt_t
#define GETTER_BOOL(t)  typename kexGetter<t>::callbackBool_t
#define GETTER_VEC2(t)  typename kexGetter<t>::callbackVec2_t
#define GETTER_VEC3(t)  typename kexGetter<t>::callbackVec3_t
#define GETTER_VEC4(t)  typename kexGetter<t>::callbackVec4_t
#define GETTER_MAT4(t)  typename kexGetter<t>::callbackMat4_t

#endif

