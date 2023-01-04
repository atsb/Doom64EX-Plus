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
// DESCRIPTION: Memory Heap Management
//
//-----------------------------------------------------------------------------

#include <assert.h>
#include "kexlib.h"

int kexHeap::numHeapBlocks = 0;
int kexHeap::currentHeapBlockID = -1;

kexHeapBlock *kexHeap::currentHeapBlock = NULL;
kexHeapBlock *kexHeap::blockList = NULL;

//
// common heap block types
//
kexHeapBlock hb_static("static", false, NULL, NULL);
kexHeapBlock hb_auto("auto", false, NULL, NULL);
kexHeapBlock hb_file("file", false, NULL, NULL);
kexHeapBlock hb_object("object", false, NULL, NULL);

//
// kexHeapBlock::kexHeapBlock
//

kexHeapBlock::kexHeapBlock(const char *name, bool bGarbageCollect,
                           blockFunc_t funcFree, blockFunc_t funcGC) {
    this->name          = (char*)name;
    this->freeFunc      = funcFree;
    this->gcFunc        = funcGC;
    this->blocks        = NULL;
    this->bGC           = bGarbageCollect;
    this->purgeID       = kexHeap::numHeapBlocks++;
    this->numAllocated  = 0;

    // add heap block to main block list
    if(kexHeap::blockList) {
        if(kexHeap::blockList->prev) {
            kexHeap::blockList->prev->next = this;
            this->prev = kexHeap::blockList->prev;
            this->next = NULL;
            kexHeap::blockList->prev = this;
        }
        else {
            kexHeap::blockList->prev = this;
            kexHeap::blockList->next = this;
            this->prev = kexHeap::blockList;
            this->next = NULL;
        }
        
    }
    else {
        kexHeap::blockList = this;
        this->prev = NULL;
        this->next = NULL;
    }
}

//
// kexHeapBlock::~kexHeapBlock
//

kexHeapBlock::~kexHeapBlock(void) {
}

//
// kexHeapBlock::operator[]
//
// Should be used with kexHeap::blockList only
//

kexHeapBlock *kexHeapBlock::operator[](int index) {
    if(kexHeap::currentHeapBlockID == index) {
        return kexHeap::currentHeapBlock;
    }

    kexHeapBlock *heapBlock = this;

    for(int i = 0; i < index; i++) {
        heapBlock = heapBlock->next;
        if(heapBlock == NULL) {
            return NULL;
        }
    }

    kexHeap::currentHeapBlockID = index;
    kexHeap::currentHeapBlock = heapBlock;

    return heapBlock;
}

//
// kexHeap::AddBlock
//

void kexHeap::AddBlock(memBlock_t *block, kexHeapBlock *heapBlock) {
    block->prev = NULL;
    block->next = heapBlock->blocks;
    heapBlock->blocks = block;

    block->heapBlock = heapBlock;

    if(block->next != NULL) {
        block->next->prev = block;
    }

    heapBlock->numAllocated++;
}

//
// kexHeap::RemoveBlock
//

void kexHeap::RemoveBlock(memBlock_t *block) {
    if(block->prev == NULL) {
        block->heapBlock->blocks = block->next;
    }
    else {
        block->prev->next = block->next;
    }

    if(block->next != NULL) {
        block->next->prev = block->prev;
    }

    block->heapBlock = NULL;
}

//
// kexHeap::GetBlock
//

memBlock_t *kexHeap::GetBlock(void *ptr, const char *file, int line) {
    memBlock_t* block;

    block = (memBlock_t*)((byte*)ptr - sizeof(memBlock_t));

    if(block->heapTag != kexHeap::HeapTag) {
        kexlib::system->Error("kexHeap::GetBlock: found a pointer without heap tag (%s:%d)", file, line);
    }

    return block;
}

//
// kexHeap::Malloc
//

void *kexHeap::Malloc(int size, kexHeapBlock &heapBlock, const char *file, int line) {
    memBlock_t *newblock;

    assert(size > 0);

    newblock = NULL;

    if(!(newblock = (memBlock_t*)malloc(sizeof(memBlock_t) + size))) {
        kexlib::system->Error("kexHeap::Malloc: failed on allocation of %u bytes (%s:%d)", size, file, line);
    }

    newblock->purgeID = heapBlock.purgeID;
    newblock->heapTag = kexHeap::HeapTag;
    newblock->size = size;
    newblock->ptrRef = NULL;
#if 0
    newblock->ms = sysMain.GetMS();
#endif

    kexHeap::AddBlock(newblock, &heapBlock);

    return ((byte*)newblock) + sizeof(memBlock_t);
}

//
// kexHeap::Calloc
//

void *kexHeap::Calloc(int size, kexHeapBlock &heapBlock, const char *file, int line) {
    return memset(kexHeap::Malloc(size, heapBlock, file, line), 0, size);
}

//
// kexHeap::Realloc
//

void *kexHeap::Realloc(void *ptr, int size, kexHeapBlock &heapBlock, const char *file, int line) {
    memBlock_t *block;
    memBlock_t *newblock;

    if(!ptr) {
        return kexHeap::Malloc(size, heapBlock, file, line);
    }

    assert(size >= 0);

    if(size == 0) {
        kexHeap::Free(ptr, file, line);
        return NULL;
    }

    block = kexHeap::GetBlock(ptr, file, line);
    newblock = NULL;

    kexHeap::RemoveBlock(block);

    block->next = NULL;
    block->prev = NULL;

    if(block->ptrRef) {
        *block->ptrRef = NULL;
    }

    if(!(newblock = (memBlock_t*)realloc(block, sizeof(memBlock_t) + size))) {
        kexlib::system->Error("kexHeap::Realloc: failed on allocation of %u bytes (%s:%d)", size, file, line);
    }

    newblock->purgeID = heapBlock.purgeID;
    newblock->heapTag = kexHeap::HeapTag;
    newblock->size = size;
    newblock->ptrRef = NULL;
#if 0
    newblock->ms = sysMain.GetMS();
#endif

    kexHeap::AddBlock(newblock, &heapBlock);

    return ((byte*)newblock) + sizeof(memBlock_t);
}

//
// kexHeap::Alloca
//

void *kexHeap::Alloca(int size, const char *file, int line) {
    return size == 0 ? NULL : kexHeap::Calloc(size, hb_auto, file, line);
}

//
// kexHeap::Free
//

void kexHeap::Free(void *ptr, const char *file, int line) {
    memBlock_t* block;

    block = kexHeap::GetBlock(ptr, file, line);

    if(block->heapTag != kexHeap::HeapTag) {
        kexlib::system->Error("kexHeap::Free: Freeing without heap tag (%s:%d)", file, line);
    }

    if(block->ptrRef) {
        *block->ptrRef = NULL;
    }

    kexHeap::RemoveBlock(block);

    // free back to system
    free(block);
}

//
// kexHeap::Purge
//

void kexHeap::Purge(kexHeapBlock &heapBlock, const char *file, int line) {
    memBlock_t *block;
    memBlock_t *next;

    for(block = heapBlock.blocks; block != NULL;) {
        next = block->next;

        if(block->heapTag != kexHeap::HeapTag) {
            kexlib::system->Error("kexHeap::Purge: Purging without heap tag (%s:%d)", file, line);
        }

        if(block->ptrRef) {
            *block->ptrRef = NULL;
        }

        free(block);
        block = next;
    }

    heapBlock.blocks = NULL;
}

//
// kexHeap::SetCacheRef
//

void kexHeap::SetCacheRef(void **ptr, const char *file, int line) {
    kexHeap::GetBlock(*ptr, file, line)->ptrRef = ptr;
}

//
// kexHeap::GarbageCollect
//

void kexHeap::GarbageCollect(const char *file, int line) {
    kexHeap::Purge(hb_auto, file, line);

#if 0
    memBlock_t *block;
    kexHeapBlock *heapBlock;

    for(heapBlock = kexHeap::blockList; heapBlock; heapBlock = heapBlock->next) {
        if(heapBlock->bGC == false) {
            continue;
        }
        for(block = heapBlock->blocks; block != NULL; block = block->next) {
            // remove block if it hasn't been used for a few seconds
            if(sysMain.GetMS() - block->ms > 10000) {
                memBlock_t *next_block = block->prev;

                kexHeap::RemoveBlock(block);
                if(heapBlock->gcFunc) {
                    heapBlock->gcFunc((byte*)block + sizeof(memBlock_t));
                }

                if(block->ptrRef) {
                    *block->ptrRef = NULL;
                }
                free(block);

                block = next_block;

                if(block == NULL) {
                    break;
                }
            }
        }
    }
#endif
}

//
// kexHeap::CheckBlocks
//

void kexHeap::CheckBlocks(const char *file, int line) {
    memBlock_t *block;
    memBlock_t *prev;
    kexHeapBlock *heapBlock;

    for(heapBlock = kexHeap::blockList; heapBlock; heapBlock = heapBlock->next) {
        prev = NULL;

        for(block = heapBlock->blocks; block != NULL; block = block->next) {
            if(block->heapTag != kexHeap::HeapTag) {
                kexlib::system->Error("kexHeap::CheckBlocks: found block without heap tag (%s:%d)", file, line);
            }
            if(block->prev != prev) {
                kexlib::system->Error("kexHeap::CheckBlocks: bad link list found (%s:%d)", file, line);
            }
            
            prev = block;
        }
    }
}

//
// kexHeap::Touch
//

void kexHeap::Touch(void *ptr, const char *file, int line) {
#if 0
    memBlock_t *block;

    block = kexHeap::GetBlock(ptr, file, line);
    block->ms = sysMain.GetMS();
#endif
}

//
// kexHeap::Usage
//

int kexHeap::Usage(const kexHeapBlock &heapBlock) {
    int bytes = 0;
    memBlock_t *block;

    for(block = heapBlock.blocks; block != NULL; block = block->next) {
        bytes += block->size;
    }

    return bytes;
}
