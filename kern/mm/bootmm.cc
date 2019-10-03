#include "sys/bootmm.h"
#include "lib/libc/string.h"
#include "lib/libcxx/new.h"
#include "sys/mmu.h"

struct run
{
    run *next;
};

struct
{
    run *freelist;
} bootmem;

extern char *_kernel_virtual_end; //kernel.ld

static void
freerange(void *vstart, void *vend)
{
    char *p = new ((char *)PGROUNDUP((size_t)vstart)) char;
    for (; p + PGSIZE <= (char *)vend; p += PGSIZE)
    {
        vm::bootmm_free(p);
    }
}

void vm::bootmm_init(void *vstart, void *vend)
{
    freerange(vstart, vend);
}

void vm::bootmm_free(char *v)
{
    //TODO: add verification such as v >= physical_mem_end
    if (((size_t)v) % PGSIZE || v < _kernel_virtual_end)
    {
        //TODO: panic the kernel for invalid free
        return;
    }

    memset(v, 1, PGSIZE);
    auto r = reinterpret_cast<run *>(v);
    r->next = bootmem.freelist;
    bootmem.freelist = r;
}

char *vm::bootmm_alloc(void)
{
    auto r = bootmem.freelist;
    if (r)
    {
        bootmem.freelist = r->next;
    }

    return reinterpret_cast<char *>(r);
}