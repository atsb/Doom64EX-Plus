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
// DESCRIPTION: File System
//
//-----------------------------------------------------------------------------

#include "kexlib.h"
#include "../framework/external/unzip.h"

#ifndef KEX_WIN32
#include <unistd.h>
#endif

#define FILE_MAX_HASH_SIZE  32768

static kexFileSystem fileSystemLocal;
kexFileSystem *kexlib::fileSystem = &fileSystemLocal;

//
// kexFileSystem::kexFileSystem
//

kexFileSystem::kexFileSystem() {
}

//
// kexFileSystem::~kexFileSystem
//

kexFileSystem::~kexFileSystem() {
}

//
// kexFileSystem::Shutdown
//

void kexFileSystem::Shutdown(void) {
    kpf_t *pack;

    kexlib::system->Printf("Shutting down file system\n");

    for(pack = root; pack; pack = pack->next) {
        unzClose(pack->filehandle);
    }

    Mem_Purge(hb_file);
}

//
// kexFileSystem::HashFileName
//

long kexFileSystem::HashFileName(const char *fname, int hashSize) const {
    unsigned int hash   = 0;
    char *str           = (char*)fname;
    char c;

    while((c = *str++)) {
        hash = c + (hash << 6) + (hash << 16) - hash;
    }

    return hash & (hashSize-1);
}

//
// kexFileSystem::LoadZipFile
//

void kexFileSystem::LoadZipFile(const char *file) {
    unzFile uf;
    unz_global_info gi;
    unz_file_info fi;
    char filename[MAX_FILEPATH];
    kpf_t *pack;
    unsigned int i;
    unsigned int entries;
    long hash;
    const char *filepath;
    kexStr fPath;

    filepath = kexlib::system->GetBaseDirectory();
    
    fPath = filepath;
    fPath = fPath + "/" + file;
    fPath.NormalizeSlashes();
    
    kexlib::system->Printf("KF_LoadZipFile: Loading %s\n", fPath.c_str());

    // open zip file
    if(!(uf = unzOpen(fPath.c_str()))) {
        kexlib::system->Error("KF_LoadZipFile: Unable to find %s", fPath.c_str());
    }

    // get info on zip file
    if(unzGetGlobalInfo(uf, &gi) != UNZ_OK) {
        return;
    }

    // allocate new pack file
    pack = (kpf_t*)Mem_Calloc(sizeof(kpf_t), hb_file);
    pack->filehandle = (unzFile*)uf;
    strcpy(pack->filename, fPath.c_str());
    pack->numfiles = gi.number_entry;
    pack->next = root;
    root = pack;

    // point to start of zip files
    unzGoToFirstFile(pack->filehandle);

    // setup hash entires
    for(entries = 1; entries < FILE_MAX_HASH_SIZE; entries <<= 1) {
        if(entries > pack->numfiles) {
            break;
        }
    }

    // allocate file/hash list
    pack->hashentries = entries;
    pack->files = (file_t*)Mem_Calloc(sizeof(file_t) * pack->numfiles, hb_file);
    pack->hashes = (file_t***)Mem_Calloc(sizeof(file_t**) * pack->hashentries, hb_file);
    pack->hashcount = (unsigned int*)Mem_Calloc(sizeof(int) * pack->hashentries, hb_file);

    // fill in file lookup lists
    for(i = 0; i < pack->numfiles; i++) {
        file_t *fp;

        if(unzGetCurrentFileInfo(uf, &fi, filename, sizeof(filename),
            NULL, 0, NULL, 0) != UNZ_OK) {
            break;
        }

        pack->files[i].cache = NULL;
        fp = &pack->files[i];

        unzGetCurrentFileInfoPosition(pack->filehandle, &fp->position);
        strcpy(fp->name, filename);
        fp->info = fi;

        // get hash number
        hash = HashFileName(filename, pack->hashentries);

        // resize hash table if needed
        pack->hashes[hash] = (file_t**)Mem_Realloc(
            pack->hashes[hash],
            sizeof(file_t*) * (pack->hashcount[hash]+1), hb_file);

        pack->hashes[hash][pack->hashcount[hash]++] = fp;

        unzGoToNextFile(pack->filehandle);
    }
}

//
// kexFileSystem::OpenFile
//

int kexFileSystem::OpenFile(const char *filename, byte **data, kexHeapBlock &hb) const {
    long hash;
    
    if(kexlib::cvarDeveloper.GetBool()) {
        int len = OpenExternalFile(filename, data);
        
        if(len != -1) {
            return len;
        }
    }
    
    for(kpf_t *pack = root; pack; pack = pack->next) {
        hash = HashFileName(filename, pack->hashentries);

        if(pack->hashes[hash]) {
            unsigned int i;

            for(i = 0; i < pack->hashcount[hash]; i++) {
                file_t *file = pack->hashes[hash][i];

                if(!strcmp(file->name, filename)) {
                    if(!file->cache) {
                        file->cache = Mem_Malloc(file->info.uncompressed_size+1, hb);
                        // automatically set cache to NULL when freed so we can
                        // recache it later
                        Mem_CacheRef(&file->cache);

                        unzSetCurrentFileInfoPosition(pack->filehandle, file->position);
                        unzOpenCurrentFile(pack->filehandle);
                        unzReadCurrentFile(pack->filehandle, file->cache,
                            file->info.uncompressed_size);
                        unzCloseCurrentFile(pack->filehandle);
                    }

                    *data = (byte*)file->cache;
                    return file->info.uncompressed_size;
                }
            }
        }
    }

    return 0;
}

//
// kexFileSystem::GetMatchingFiles
//

void kexFileSystem::GetMatchingFiles(kexStrList &list, const char *search) {
    if(kexlib::cvarDeveloper.GetBool()) {
        DIR *dir;
        struct dirent *ent;
        int idx;
        kexStr path = kexStr(kexlib::cvarBasePath.GetValue()) + "/" + search;
        path.NormalizeSlashes();
        
        if((dir = opendir(path.c_str())) != NULL) {
            while((ent = readdir(dir)) != NULL) {
                idx = kexStr::IndexOf(ent->d_name, ".");
                if(idx == -1 || idx == 0) {
                    continue;
                }
                list.Push(kexStr(ent->d_name));
            }
            closedir(dir);
        }
    }
    
    for(kpf_t *pack = root; pack; pack = pack->next) {
        for(unsigned int i = 0; i < pack->numfiles; i++) {
            file_t *file = &pack->files[i];

            if(strstr(file->name, search)) {
                if(kexStr::IndexOf(file->name, ".") == -1) {
                    continue;
                }

                list.Push(kexStr(file->name));
            }
        }
    }
}

//
// kexFileSystem::OpenExternalFile
//

int kexFileSystem::OpenExternalFile(const char *name, byte **buffer) const {
    static char filepath[1024];
    kexStr fPath;
    FILE *fp;

    sprintf(filepath, "%s\\%s", kexlib::cvarBasePath.GetValue(), name);
    
    fPath = filepath;
    fPath.NormalizeSlashes();
    
    if((fp = fopen(fPath.c_str(), "rb"))) {
        size_t length;

        fseek(fp, 0, SEEK_END);
        length = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        *buffer = (byte*)Mem_Calloc(length+1, hb_file);

        if(fread(*buffer, 1, length, fp) == length) {
            fclose(fp);
            return length;
        }
        
        Mem_Free(*buffer);
        *buffer = NULL;
        fclose(fp);
   }
   
   return -1;
}

//
// kexFileSystem::Init
//

void kexFileSystem::Init(void) {
    if(!strlen(kexlib::cvarBasePath.GetValue())) {
        kexlib::cvarBasePath.Set(kexlib::system->GetBaseDirectory());
    }
}
