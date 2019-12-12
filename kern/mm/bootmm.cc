/**
 * @ Author: SmartPolarBear
 * @ Create Time: 1970-01-01 08:00:00
 * @ Modified by: SmartPolarBear
 * @ Modified time: 2019-12-12 23:09:18
 * @ Description:
 */

//FIXME: some fault occurs in multiboot.cc for the mem is unexpectedly modified.

#include "sys/bootmm.h"
#include "drivers/debug/kdebug.h"
#include "lib/libc/string.h"
#include "lib/libcxx/new"
#include "sys/memlayout.h"
#include "sys/mmu.h"

using vm::BOOTMM_BLOCKSIZE;

struct run
{
    run *next;
};

struct
{
    run *freelist;
} bootmem;

extern char end[]; //kernel.ld

static void
freerange(void *vstart, void *vend)
{
    char *p = (char *)PAGE_ROUNDUP((uintptr_t)vstart);
    for (; p + BOOTMM_BLOCKSIZE <= reinterpret_cast<char *>(PAGE_ROUNDDOWN((uintptr_t)vend)); p += BOOTMM_BLOCKSIZE)
    {
        vm::bootmm_free(p);
    }
}

//Ininialzie the boot mem manager
void vm::bootmm_init(void *vstart, void *vend)
{
    freerange(vstart, vend);
}

//Free a block
void vm::bootmm_free(char *v)
{
    if (((size_t)v) % BOOTMM_BLOCKSIZE || v < end || V2P((uintptr_t)v) >= PHYMEMORY_SIZE)
    {
        KDEBUG_GENERALPANIC("Unable to free an invalid pointer.\n");
    }

    memset(v, 1, BOOTMM_BLOCKSIZE);
    auto r = reinterpret_cast<run *>(v);
    r->next = bootmem.freelist;
    bootmem.freelist = r;
}

// Allocate a block sized 4096 bytes
char *vm::bootmm_alloc(void)
{
    auto r = bootmem.freelist;
    if (r)
    {
        bootmem.freelist = r->next;
    }

    return reinterpret_cast<char *>(r);
}