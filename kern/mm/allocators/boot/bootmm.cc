/*
 * Last Modified: Sun Feb 02 2020
 * Modified By: SmartPolarBear
 * -----
 * Copyright (C) 2006 by SmartPolarBear <clevercoolbear@outlook.com>
 * 
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 * -----
 * HISTORY:
 * Date      	By	Comments
 * ----------	---	----------------------------------------------------------
 */

//FIXME: some fault occurs in multiboot.cc for the mem is unexpectedly modified.

#include "sys/allocators/boot_alloc.h"
#include "sys/memlayout.h"
#include "sys/mmu.h"

#include "lib/libc/string.h"
#include "lib/libcxx/new"

#include "drivers/debug/kdebug.h"

using allocators::boot_allocator::BOOTMM_BLOCKSIZE;

struct run
{
    run *next;
};

struct
{
    run *freelist;
    size_t used;
} bootmem;

extern char end[]; //kernel.ld

static inline void
freerange(void *vstart, void *vend)
{
    char *p = (char *)PAGE_ROUNDUP((uintptr_t)vstart);
    for (; p + BOOTMM_BLOCKSIZE <= reinterpret_cast<char *>(PAGE_ROUNDDOWN((uintptr_t)vend)); p += BOOTMM_BLOCKSIZE)
    {
        allocators::boot_allocator::bootmm_free(p);
    }
}

//Ininialzie the boot mem manager
void allocators::boot_allocator::bootmm_init(void *vstart, void *vend)
{
    freerange(vstart, vend);
    bootmem.used = 0;
}

//Free a block
void allocators::boot_allocator::bootmm_free(char *v)
{
    if (((size_t)v) % BOOTMM_BLOCKSIZE || v < end || V2P((uintptr_t)v) >= PHYMEMORY_SIZE)
    {
        KDEBUG_RICHPANIC("Unable to free an invalid pointer.\n",
                         "KERNEL PANIC: BOOTMM",
                         false,
                         "");
    }

    bootmem.used--;
    KDEBUG_ASSERT(bootmem.used >= 0 && bootmem.used <= vm::BOOT_MEM_LIMIT);

    memset(v, 1, BOOTMM_BLOCKSIZE);
    auto r = reinterpret_cast<run *>(v);
    r->next = bootmem.freelist;
    bootmem.freelist = r;
}

// Allocate a block sized 4096 bytes
char *allocators::boot_allocator::bootmm_alloc(void)
{
    auto r = bootmem.freelist;
    if (r)
    {
        bootmem.freelist = r->next;
    }

    bootmem.used++;

    return reinterpret_cast<char *>(r);
}

size_t allocators::boot_allocator::bootmm_get_used(void)
{
    return bootmem.used;
}
