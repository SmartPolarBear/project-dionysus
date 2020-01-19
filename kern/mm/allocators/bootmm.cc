/*
 * Last Modified: Sat Jan 18 2020
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