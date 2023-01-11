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

#ifndef __CVAR_H__
#define __CVAR_H__

//
// CVARS
//
typedef enum
{
    CVF_BOOL        = BIT(0),
    CVF_INT         = BIT(1),
    CVF_FLOAT       = BIT(2),
    CVF_STRING      = BIT(3),
    CVF_NETWORK     = BIT(4),
    CVF_CHEAT       = BIT(5),
    CVF_CONFIG      = BIT(6),
    CVF_ALLOCATED   = BIT(7)
} cvarFlags_t;

class kexCvar {
public:
                    kexCvar(const char *name, int flags, const char *value, const char *description);
                    kexCvar(const char *name, int flags, const char *value,
                        float min, float max, const char *description);

    const char      *GetName(void) const { return name; }
    const char      *GetDescription(void) const { return description; }
    const char      *GetDefaultValue(void) const { return defaultValue; }
    int             GetFlags(void) const { return flags; }
    float           GetMin(void) const { return min; }
    float           GetMax(void) const { return max; }
    const char      *GetValue(void) const { return value; }
    bool            GetBool(void) const { return (atoi(value) > 0); }
    int             GetInt(void) const { return atoi(value); }
    float           GetFloat(void) const { return (float)atof(value); }
    void            SetNext(kexCvar *cvar) { next = cvar; }
    kexCvar         *GetNext(void) const { return next; }
    bool            IsModified(void) const { return bModified; }
    void            FreeStringValue(void);
    void            SetNewStringValue(const char *string);
    void            Set(const char *string);
    void            Set(int value);
    void            Set(float value);

private:
    void            Setup(const char *name, const char *value, const char *description,
                        int flags, float min, float max);
protected:
    const char      *name;
    char            *value;
    const char      *description;
    const char      *defaultValue;
    int             flags;
    float           min;
    float           max;
    bool            bModified;
    kexCvar         *next;
};

class kexCvarManager {
public:
    void            Init(void);
    void            Register(kexCvar *variable);
    kexCvar         *Get(const char *name);
    void            Set(const char *var_name, const char *value);
    void            Set(const char *var_name, float value);
    void            Set(const char *var_name, int value);
    void            AutoComplete(const char *partial);
    void            WriteToFile(FILE *file);
    kexCvar         *GetFirst(void) const { return first; }
    void            Shutdown(void);

private:
    kexCvar         *first;
    kexCvar         *next;
};

#endif
