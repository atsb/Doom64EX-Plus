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

#ifndef __FILESYSTEM_H__
#define __FILESYSTEM_H__

#include "../framework/external/unzip.h"

class kexFileSystem {
public:
                        kexFileSystem();
                        ~kexFileSystem();

    void                Shutdown(void);
    void                LoadZipFile(const char *file);
    int                 OpenFile(const char *filename, byte **data, kexHeapBlock &hb) const;
    int                 OpenExternalFile(const char *name, byte **buffer) const;
    void                GetMatchingFiles(kexStrList &list, const char *search);
    void                Init(void);

private:
    long                HashFileName(const char *fname, int hashSize) const;

    typedef struct {
        char            name[MAX_FILEPATH];
        unsigned long   position;
        unz_file_info   info;
        void*           cache;
    } file_t;

    typedef struct kpf_s {
        unzFile         *filehandle;
        unsigned int    numfiles;
        char            filename[MAX_FILEPATH];
        file_t          *files;
        file_t          ***hashes;
        unsigned int    *hashcount;
        unsigned int    hashentries;
        struct kpf_s    *next;
    } kpf_t;

    kpf_t               *root;
    char                *base;
};

#endif
