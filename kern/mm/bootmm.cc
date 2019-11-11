#include "sys/bootmm.h"
#include "drivers/debug/kdebug.h"
#include "lib/libc/string.h"
#include "lib/libcxx/new"
#include "sys/mmu.h"
constexpr size_t PGSIZE = 4096;

static inline constexpr size_t PGROUNDUP(size_t sz)
{
    return (((sz) + ((size_t)PGSIZE - 1ul)) & ~((size_t)(PGSIZE - 1ul)));
}


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
    char *p = (char *)PGROUNDUP((uintptr_t)vstart);
    for (; p + PGSIZE <= (char *)vend; p += PGSIZE)
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
    //TODO: add verification such as v >= physical_mem_end
    if (((size_t)v) % PGSIZE || v < end)
    {
        KDEBUG_GENERALPANIC("Unable to free a pointer that is out of range.");
    }

    memset(v, 1, PGSIZE);
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