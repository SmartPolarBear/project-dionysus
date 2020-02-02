#include "sys/pmm.h"
#include "sys/memlayout.h"
#include "sys/mmu.h"
#include "sys/multiboot.h"

#include "drivers/console/console.h"

#include "lib/libcxx/algorithm"

using pmm::page_count;
using pmm::pages;
using pmm::pmm_manager;

pmm::pmm_manager_info *pmm::pmm_manager;

page_info *pmm::pages;
size_t pmm::page_count;

static inline void init_pmm_manager()
{
    // set the physical memory manager
    // pmm_manager_info=&
    console::printf("Initialized PMM %s\n", pmm_manager->name);
    pmm_manager->init();
}

static inline void init_memmap(page_info *base, size_t sz)
{
    pmm_manager->init_memmap(base, sz);
}

static inline void detect_physical_mem()
{
    auto memtag = multiboot::acquire_tag_ptr<multiboot_tag_mmap>(MULTIBOOT_TAG_TYPE_MMAP);
    size_t entry_count = (memtag->size - sizeof(multiboot_uint32_t) * 4ul - sizeof(memtag->entry_size)) / memtag->entry_size;

    auto max_pa = 0ull;

    for (size_t i = 0; i < entry_count; i++)
    {
        const auto entry = memtag->entries + i;
        if (entry->type == MULTIBOOT_MEMORY_AVAILABLE)
        {
            max_pa = sysstd::max(max_pa, sysstd::min(entry->addr + entry->len, (unsigned long long)PHYMEMORY_SIZE));
        }
    }

    page_count = max_pa / PHYSICAL_PAGE_SIZE;

    size_t pages_page = (page_count * sizeof(page_info)) / PHYSICAL_PAGE_SIZE;
    
}

void pmm::init_pmm(void)
{
    init_pmm_manager();
}

page_info *pmm::alloc_pages(size_t n)
{
}

void pmm::free_pages(page_info *base, size_t n)
{
}

size_t pmm::free_page_cound(void)
{
}
