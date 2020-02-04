/*
 * Last Modified: Tue Feb 04 2020
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

#include "arch/amd64/sync.h"

#include "sys/memlayout.h"
#include "sys/mmu.h"
#include "sys/multiboot.h"
#include "sys/pmm.h"

#include "drivers/console/console.h"

#include "lib/libc/stdlib.h"
#include "lib/libcxx/algorithm"
#include "lib/libcxx/utility"

using pmm::page_count;
using pmm::pages;
using pmm::pmm_manager;

using sysstd::simple_pair;

using reserved_space = simple_pair<uintptr_t, uintptr_t>;

extern uint8_t end[];                     // kernel.ld
extern "C" void *mbi_structptr = nullptr; // multiboot.cc

pmm::pmm_manager_info *pmm::pmm_manager;

page_info *pmm::pages;
size_t pmm::page_count;

constexpr size_t RESERVED_SPACES_MAX_COUNT = 32;
size_t reserved_spaces_count = 0;
reserved_space reserved_spaces[RESERVED_SPACES_MAX_COUNT];

// the count of module tags can't be more than reserved spaces
multiboot::multiboot_tag_ptr module_tags[RESERVED_SPACES_MAX_COUNT];

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
    // The page management structure is placed a page after kernel
    // So as to protect the multiboot info
    pages = (page_info *)roundup((uintptr_t)(end + PHYSICAL_PAGE_SIZE), PHYSICAL_PAGE_SIZE);

    for (size_t i = 0; i < page_count; i++)
    {
        pages[i].flags &= PHYSICAL_PAGE_FLAG_RESERVED; // set all pages as reserved
    }

    uintptr_t physical_available_begin = P2V((uintptr_t)(&pages[page_count]));

    // reserve the multiboot info
    if ((uintptr_t)mbi_structptr >= physical_available_begin)
    {
        reserved_spaces[reserved_spaces_count++] =
            reserved_space{.first = (uintptr_t)mbi_structptr,
                           .second = (uintptr_t)mbi_structptr + PHYSICAL_PAGE_SIZE};
    }

    // reserve all the boot modules
    size_t module_count = multiboot::get_all_tags(MULTIBOOT_TAG_TYPE_MODULE, module_tags, RESERVED_SPACES_MAX_COUNT);
    for (size_t i = 0; i < module_count; i++)
    {
        multiboot_tag_module *tag = reinterpret_cast<decltype(tag)>(module_tags[i]);
        if (tag->mod_end >= physical_available_begin)
        {
            reserved_spaces[reserved_spaces_count++] =
                reserved_space{.first = (uintptr_t)tag->mod_start,
                               .second = (uintptr_t)tag->mod_end};
        }
    }

    qsort(reserved_spaces, reserved_spaces_count, sizeof(reserved_spaces[0]), [](const void *a, const void *b) -> int {
        return ((reserved_space *)a)->first - ((reserved_space *)b)->first;
    });

    auto pa_to_page = [physical_available_begin](uintptr_t pa) -> page_info * {
        size_t index = (pa - physical_available_begin) / PHYSICAL_PAGE_SIZE;
        return &pages[index];
    };

    auto count_of_pages = [](uintptr_t st, uintptr_t ed) {
        return (ed - st) / PHYSICAL_PAGE_SIZE;
    };

    for (int i = reserved_spaces_count - 1; i >= 0; i--)
    {
        if (((size_t)i) == reserved_spaces_count - 1)
        {
            init_memmap(pa_to_page(reserved_spaces[i].second),
                        count_of_pages(reserved_spaces[i].second, max_pa));
        }
        else if (i == 0)
        {
            init_memmap(pa_to_page(physical_available_begin),
                        count_of_pages(physical_available_begin, reserved_spaces[i].first));
        }
        else
        {
            init_memmap(pa_to_page(reserved_spaces[i].second),
                        count_of_pages(reserved_spaces[i].second, reserved_spaces[i + 1].first));
        }
    }
}

void pmm::init_pmm(void)
{
    init_pmm_manager();
}

page_info *pmm::alloc_pages(size_t n)
{
    bool intrrupt_flag = false;
    page_info *ret = nullptr;

    local_intrrupt_save(intrrupt_flag);

    ret = pmm_manager->alloc_pages(n);

    local_intrrupt_restore(intrrupt_flag);

    return ret;
}

void pmm::free_pages(page_info *base, size_t n)
{
    bool intrrupt_flag = false;

    local_intrrupt_save(intrrupt_flag);

    pmm_manager->free_pages(base, n);

    local_intrrupt_restore(intrrupt_flag);
}

size_t pmm::get_free_page_count(void)
{
    bool intrrupt_flag = false;
    size_t ret = 0;

    local_intrrupt_save(intrrupt_flag);

    ret = pmm_manager->get_free_pages_count();

    local_intrrupt_restore(intrrupt_flag);

    return ret;
}
