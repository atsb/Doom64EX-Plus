//
// Copyright (c) 2013-2014 Samuel Villarreal
// svkaiser@gmail.com
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
//    1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
//
//   2. Altered source versions must be plainly marked as such, and must not be
//   misrepresented as being the original software.
//
//    3. This notice may not be removed or altered from any source
//    distribution.
//

#ifndef __KSTRING_H__
#define __KSTRING_H__

#include <string.h>
#include "common.h"
#include "array.h"

class kexStr;

typedef kexArray<kexStr>        kexStrList;

class kexStr
{
public:
    kexStr(void);
    kexStr(const char *string);
    kexStr(const char *string, const int length);
    kexStr(const kexStr &string);
    ~kexStr(void);

    void                CheckSize(int size, bool bKeepString);
    int                 IndexOf(const char *pattern) const;
    int                 IndexOf(const kexStr &pattern) const;
    kexStr              &Concat(const char *string);
    kexStr              &Concat(const char *string, int len);
    kexStr              &Concat(const char c);
    kexStr              &NormalizeSlashes(void);
    kexStr              &StripPath(void);
    kexStr              &StripExtension(void);
    kexStr              &StripFile(void);
    kexStr              &Copy(const kexStr &src, int len);
    kexStr              &Copy(const kexStr &src);
    kexStr              &ToUpper(void);
    kexStr              &ToLower(void);
    int                 Hash(void);
    kexStr              Substr(int start, int len) const;
    void                Split(kexStrList &list, const char seperator);
    int                 Atoi(void);
    float               Atof(void);
    void                WriteToFile(const char *file);

    int                 Length(void) const { return length; }
    const char          *c_str(void) const { return charPtr; }

    kexStr              &operator=(const kexStr &str);
    kexStr              &operator=(const char *str);
    kexStr              &operator=(const bool b);
    kexStr              operator+(const kexStr &str);
    kexStr              operator+(const char *str);
    kexStr              operator+(const bool b);
    kexStr              operator+(const int i);
    kexStr              operator+(const float f);
    kexStr              &operator+=(const kexStr &str);
    kexStr              &operator+=(const char *str);
    kexStr              &operator+=(const char c);
    kexStr              &operator+=(const bool b);
    const char          operator[](int index) const;

    friend bool         operator==(const kexStr &a, const kexStr &b);
    friend bool         operator==(const char *a, const kexStr &b);
    friend bool         operator==(const kexStr &a, const char *b);

    operator            const char *(void) const { return c_str(); }
    operator            const char *(void) { return c_str(); }

    static bool         CompareCase(const char *s1, const char *s2);
    static bool         CompareCase(const kexStr &a, const kexStr &b);
    static bool         Compare(const char *s1, const char *s2);
    static bool         Compare(const kexStr &a, const kexStr &b);
    static int          IndexOf(const char *string, const char *pattern);
    static int          Hash(const char *s);
    static char         *Format(const char *str, ...);

private:
    void                Resize(int size, bool bKeepString);
    void                CopyNew(const char *string, int len);

protected:
    void                Init(void);

    static const int    STRING_DEFAULT_SIZE = 32;

    char                *charPtr;
    char                defaultBuffer[STRING_DEFAULT_SIZE];
    int                 length;
    int                 bufferLength;
};

d_inline bool operator==(const kexStr &a, const kexStr &b)
{
    return (!strcmp(a.charPtr, b.charPtr));
}

d_inline bool operator==(const char *a, const kexStr &b)
{
    return (!strcmp(a, b.charPtr));
}

d_inline bool operator==(const kexStr &a, const char *b)
{
    return (!strcmp(a.charPtr, b));
}

#endif
