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

#ifndef __MEM_HEAP_H__
#define __MEM_HEAP_H__

typedef void (*blockFunc_t)(void*);

class kexHeapBlock;

typedef struct memBlock_s {
    int                     heapTag;
    int                     purgeID;
    int                     size;
    int                     ms;
    kexHeapBlock            *heapBlock;
    void                    **ptrRef;
    struct memBlock_s       *prev;
    struct memBlock_s       *next;
} memBlock_t;

class kexHeapBlock {
public:
                            kexHeapBlock(const char *name, bool bGarbageCollect,
                                blockFunc_t funcFree, blockFunc_t funcGC);
                            ~kexHeapBlock(void);

    kexHeapBlock            *operator[](int index);
    
    char                    *name;
    memBlock_t              *blocks;
    bool                    bGC;
    blockFunc_t             freeFunc;
    blockFunc_t             gcFunc;
    int                     purgeID;
    int                     numAllocated;
    kexHeapBlock            *prev;
    kexHeapBlock            *next;
};

class kexHeap {
public:
    static void             *Malloc(int size, kexHeapBlock &heapBlock, const char *file, int line);
    static void             *Calloc(int size, kexHeapBlock &heapBlock, const char *file, int line);
    static void             *Realloc(void *ptr, int size, kexHeapBlock &heapBlock, const char *file, int line);
    static void             *Alloca(int size, const char *file, int line);
    static void             Free(void *ptr, const char *file, int line);
    static void             Purge(kexHeapBlock &heapBlock, const char *file, int line);
    static void             GarbageCollect(const char *file, int line);
    static void             CheckBlocks(const char *file, int line);
    static void             Touch(void *ptr, const char *file, int line);
    static int              Usage(const kexHeapBlock &heapBlock);
    static void             SetCacheRef(void **ptr, const char *file, int line);

    static int              numHeapBlocks;
    static kexHeapBlock     *currentHeapBlock;
    static int              currentHeapBlockID;
    static kexHeapBlock     *blockList;

private:
    static void             AddBlock(memBlock_t *block, kexHeapBlock *heapBlock);
    static void             RemoveBlock(memBlock_t *block);
    static memBlock_t       *GetBlock(void *ptr, const char *file, int line);
    
    static const int        HeapTag = 0x03151983;
};

extern kexHeapBlock hb_static;
extern kexHeapBlock hb_auto;
extern kexHeapBlock hb_file;
extern kexHeapBlock hb_object;

#define Mem_Malloc(s, hb)       (kexHeap::Malloc(s, hb, __FILE__,__LINE__))
#define Mem_Calloc(s, hb)       (kexHeap::Calloc(s, hb, __FILE__,__LINE__))
#define Mem_Realloc(p, s, hb)   (kexHeap::Realloc(p, s, hb, __FILE__,__LINE__))
#define Mem_Alloca(s)           (kexHeap::Alloca(s, __FILE__,__LINE__))
#define Mem_Free(p)             (kexHeap::Free(p, __FILE__,__LINE__))
#define Mem_Purge(hb)           (kexHeap::Purge(hb, __FILE__,__LINE__))
#define Mem_GC()                (kexHeap::GarbageCollect(__FILE__,__LINE__))
#define Mem_CheckBlocks()       (kexHeap::CheckBlocks(__FILE__,__LINE__))
#define Mem_Touch(p)            (kexHeap::Touch(p, __FILE__,__LINE__))
#define Mem_CacheRef(p)         (kexHeap::SetCacheRef(p, __FILE__,__LINE__))

#define Mem_AllocStatic(s)      (Mem_Calloc(s, hb_static))

#define Mem_Strdup(s, hb)       (strcpy((char*)Mem_Malloc(strlen(s)+1, hb), s))
#define Mem_Strdupa(s)          (strcpy((char*)Mem_Alloca(strlen(s)+1), s))

#endif
