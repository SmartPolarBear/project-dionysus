/**
 * @ Author: SmartPolarBear
 * @ Create Time: 2019-10-13 22:46:26
 * @ Modified by: Daniel Lin
 * @ Modified time: 2020-01-16 18:01:06
 * @ Description: Implement Intel's 4-level paging and the modification of page tables, etc.
 */

#include "sys/vm.h"

#include "boot/multiboot2.h"

#include "sys/bootmm.h"
#include "sys/error.h"
#include "sys/memlayout.h"
#include "sys/mmu.h"
#include "sys/multiboot.h"
#include "sys/param.h"
#include "sys/types.h"

#include "arch/amd64/x86.h"

#include "drivers/acpi/cpu.h"
#include "drivers/apic/apic.h"
#include "drivers/apic/traps.h"
#include "drivers/console/console.h"
#include "drivers/debug/kdebug.h"

#include "lib/libc/string.h"
#include "lib/libcxx/algorithm"

using vm::pde_ptr_t;
using vm::pde_t;

using vm::bootmm_alloc;

/*TODO: kernel text should be specially mapped for the sake of safety
    this should be done after we have user processes to prevent their attempt to change kernel code*/
enum kmem_map_index
{
    KMMAP_KERNEL,
    KMMAP_DEV,
    KMMAP_PHYREMAP
};

struct
{
    uintptr_t pstart, pend, vstart;
    size_t permissions;
} kmem_map[] =
    {
        // map kernel from address 0 to KERNEL_VIRTUALBASE
        [KMMAP_KERNEL] =
            {
                .pstart = 0,
                .pend = KERNEL_SIZE,
                .vstart = KERNEL_VIRTUALBASE,
                .permissions = PG_W},
        // map memory for memory-mapped I/O
        [KMMAP_DEV] =
            {
                .pstart = DEVICE_PHYSICALBASE,
                .pend = DEVICE_PHYSICALEND,
                .vstart = DEVICE_VIRTUALBASE,
                .permissions = PG_W | PG_PWT | PG_PCD},
        // map whole physical memory again to PHYREMAP_VIRTUALBASE
        [KMMAP_PHYREMAP] =
            {
                .pstart = 0,
                .pend = 0, //to be filled in initialize_phymem_parameters
                .vstart = PHYREMAP_VIRTUALBASE,
                .permissions = PG_W}};

[[maybe_unused]] constexpr auto kmem_map_size = (sizeof(kmem_map) / sizeof(kmem_map[0]));

struct
{
    size_t physize;
    struct
    {
        multiboot_mmap_entry *entries;
        size_t size;
    } memmap;
} phy_mem_info;

// global kmpl4t ptr for the convenience of access
static pde_ptr_t g_kpml4t;

static inline RESULT map_addr(const pde_ptr_t kpml4t, uintptr_t vaddr, uintptr_t paddr, size_t permissions)
{
    // the lambda removes all flags in entries to expose the address of the next level
    auto remove_flags = [](size_t pde) {
        constexpr size_t FLAGS_SHIFT = 8;
        return (pde >> FLAGS_SHIFT) << FLAGS_SHIFT;
    };

    // the kpml4, which's the content of CR3, is held in kpml4t
    // firstly find the 3rd page directory (PDPT) from it.
    pde_ptr_t pml4e = &kpml4t[P4X(vaddr)],
              pdpt = nullptr,
              pdpte = nullptr,
              pgdir = nullptr,
              pde = nullptr;

    if (!(*pml4e & PG_P))
    {
        pdpt = reinterpret_cast<pde_ptr_t>(bootmm_alloc());
        KDEBUG_ASSERT(pdpt != nullptr);
        if (pdpt == nullptr)
        {
            return ERROR_MEMORY_ALLOC;
        }

        memset(pdpt, 0, PAGE_SIZE);
        *pml4e = ((V2P((uintptr_t)pdpt)) | PG_P | permissions);
    }
    else
    {
        pdpt = reinterpret_cast<decltype(pdpt)>(P2V(remove_flags(*pml4e)));
    }

    // find the 2nd page directory from PGPT
    pdpte = &pdpt[P3X(vaddr)];
    if (!(*pdpte & PG_P))
    {
        pgdir = reinterpret_cast<pde_ptr_t>(bootmm_alloc());
        KDEBUG_ASSERT(pgdir != nullptr);
        if (pgdir == nullptr)
        {
            return ERROR_MEMORY_ALLOC;
        }

        memset(pgdir, 0, PAGE_SIZE);
        *pdpte = ((V2P((uintptr_t)pgdir)) | PG_P | permissions);
    }
    else
    {
        pgdir = reinterpret_cast<decltype(pgdir)>(P2V(remove_flags(*pdpte)));
    }

    // find the page from 2nd page directory.
    // because we use page size extension (2mb pages)
    // this is the last level.
    pde = &pgdir[P2X(vaddr)];
    if (!(*pde & PG_P))
    {
        *pde = ((paddr) | PG_PS | PG_P | permissions);
    }

    return ERROR_SUCCESS;
}

static inline void check_hardware(void)
{
    uint64_t regs[4];
    cpuid(CPUID_GETFEATURES, regs);
    if (!(regs[3] & 0x00000040))
    {
        KDEBUG_GENERALPANIC("CPU Feature 'PAE' is required for 4-level paging.");
    }
}

static inline void initialize_phymem_parameters(void)
{
    // get mmap tag from multiboot info and find the limit of physical memory
    auto memtag = multiboot::aquire_tag_ptr<multiboot_tag_mmap>(MULTIBOOT_TAG_TYPE_MMAP);
    size_t entry_count = (memtag->size - sizeof(multiboot_uint32_t) * 4ul - sizeof(memtag->entry_size)) / memtag->entry_size;
    auto physize = 0ull;

    for (size_t i = 0; i < entry_count; i++)
    {
        auto entry = memtag->entries + i;
        physize = sysstd::max(physize, sysstd::min(entry->addr + entry->len, (unsigned long long)PHYMEMORY_SIZE));
    }

    physize = PAGE_ROUNDDOWN(physize);

    KDEBUG_ASSERT(physize >= KERNEL_SIZE && physize <= PHYMEMORY_SIZE);

    phy_mem_info.memmap.entries = memtag->entries;
    phy_mem_info.memmap.size = entry_count;

    phy_mem_info.physize = physize;

    //complete the memory map
    kmem_map[KMMAP_PHYREMAP].pend = physize;
}

static hresult handle_pgfault(trap_info info)
{
    console::printf("page fault!\n");
    KDEBUG_FOLLOWPANIC("page fault");
    return HRES_SUCCESS;
}

void vm::switch_kernelvm()
{
    lcr3(V2P((uintptr_t)g_kpml4t));
}

pde_t *vm::setup_kernelvm(void)
{
    KDEBUG_NOT_IMPLEMENTED;
    return nullptr;
}

void vm::init_kernelvm(void)
{
    // check the feature availablity
    check_hardware();

    // cache significant information in advance
    initialize_phymem_parameters();

    // allocate pml4t and initialize it with 0
    g_kpml4t = reinterpret_cast<pde_ptr_t>(bootmm_alloc());
    memset(g_kpml4t, 0, PAGE_SIZE);

    // map all the definition in kmem_map
    for (auto kmmap_entry : kmem_map)
    {
        for (uintptr_t addr = kmmap_entry.pstart, virtual_addr = kmmap_entry.vstart;
             addr <= kmmap_entry.pend;
             addr += PG_PS_SIZE, virtual_addr += PG_PS_SIZE)
        {
            if (map_addr(g_kpml4t, virtual_addr, addr, kmmap_entry.permissions) != ERROR_SUCCESS)
            {
                KDEBUG_GENERALPANIC("Can't allocate enough space for paging.\n");
            }
        }
    }

    // install the PML4T to the CR3 register.
    switch_kernelvm();

    trap::trap_handle_regsiter(trap::TRAP_PGFLT, trap::trap_handle{
                                                     .handle = handle_pgfault});
}

void vm::freevm(pde_t *pgdir)
{
    [[maybe_unused]] auto warning_bypass = pgdir;
}
