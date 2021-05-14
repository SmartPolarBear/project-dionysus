
// Copyright (c) 2021 SmartPolarBear
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "include/pmm.h"

#include "memory/pmm.hpp"

#include "arch/amd64/cpu/x86.h"

#include "system/error.hpp"
#include "system/kmem.hpp"
#include "system/memlayout.h"
#include "system/mmu.h"
#include "system/multiboot.h"
#include "system/pmm.h"
#include "system/vmm.h"
#include "system/segmentation.hpp"

#include "drivers/apic/traps.h"
#include "drivers/console/console.h"
#include "debug/kdebug.h"

#include "memory/pmm.hpp"

#include "../../libs/basic_io/include/builtin_text_io.hpp"

#include "kbl/lock/lock_guard.hpp"

#include "ktl/span.hpp"
#include "ktl/pair.hpp"
#include "ktl/algorithm.hpp"

#include <algorithm>
#include <utility>

#include <gsl/util>

using namespace memory;

using pmm::page_count;
using pmm::pages;


using vmm::pde_ptr_t;

using namespace ktl;

// TODO: use lock
// TODO: i need to manually enable locks because popcli and pushcli relies on gdt

using reserved_space = pair<uintptr_t, uintptr_t>;

page* pmm::pages = nullptr;
size_t pmm::page_count = 0;

constexpr size_t RESERVED_SPACES_MAX_COUNT = 32;
size_t reserved_spaces_count = 0;
reserved_space reserved_spaces[RESERVED_SPACES_MAX_COUNT] = {};

// the count of module tags can't be more than reserved spaces
multiboot::multiboot_tag_ptr module_tags[RESERVED_SPACES_MAX_COUNT];

static inline void init_pmm_manager()
{
	if (!memory::physical_memory_manager::instance()->is_well_constructed())
	{
		KDEBUG_GENERALPANIC("Can't initialize physical_memory_manager");
	}
}

static inline void init_memmap(page* base, size_t sz)
{
//	pmm_entity->init_memmap(base, sz);
	physical_memory_manager::instance()->setup_for_base(base, sz);

}

/*
The memory layout for kernel:
......
| available     |
| memory        |
|               |
-------------------------- pavailable_start()
(align to 4K)
--------------------------
|               |
| pages of pmm  |
|               |
|               |
|               |
|               |
-------------------------- end+4MB
| Two           |
| physical      |
| pages for     |
| multiboot     |
| information   |
-------------------------- end
|               |
|  kernel       |
|               |
-------------------------- KERNEL_VIRTUALBASE
......
*/

static inline constexpr auto count_of_pages(uintptr_t st, uintptr_t ed)
{
	return (ed - st) / PAGE_SIZE;
}

static inline void init_physical_mem()
{
	auto basic_mem = multiboot::acquire_tag_ptr<multiboot_tag_basic_meminfo>(MULTIBOOT_TAG_TYPE_BASIC_MEMINFO);
	auto max_pa = static_cast<uint64_t>(basic_mem->mem_upper + basic_mem->mem_lower) << 10ull;

	page_count = max_pa / PAGE_SIZE;

	// FIXME: We may not need to do this ?
	// The page management structure is placed a page after kernel
	// So as to protect the multiboot info
	pages = (page*)roundup((uintptr_t)(end + PAGE_SIZE * 2), PAGE_SIZE);

	for (size_t i = 0; i < page_count; i++)
	{
		pages[i].flags |= PHYSICAL_PAGE_FLAG_RESERVED; // set all pages as reserved
	}

	auto memtag = multiboot::acquire_tag_ptr<multiboot_tag_mmap>(MULTIBOOT_TAG_TYPE_MMAP);
	span<multiboot_mmap_entry> entries{ memtag->entries,
	                                    (memtag->size - sizeof(multiboot_uint32_t) * 4ul - sizeof(memtag->entry_size))
		                                    / memtag->entry_size };

	// reserve all the boot modules
	size_t module_count = multiboot::get_all_tags(MULTIBOOT_TAG_TYPE_MODULE, module_tags, RESERVED_SPACES_MAX_COUNT);
	for (size_t i = 0; i < module_count; i++)
	{
		multiboot_tag_module* tag = reinterpret_cast<decltype(tag)>(module_tags[i]);
		if (tag->mod_end >= pmm::pavailable_start())
		{
			reserved_spaces[reserved_spaces_count++] =
				std::make_pair((uintptr_t)tag->mod_start, (uintptr_t)tag->mod_end);
		}
	}

	sort(reserved_spaces, reserved_spaces + reserved_spaces_count, [](const auto& a, const auto& b)
	{
	  return a.first < b.first;
	});

	if (reserved_spaces_count == 0)
	{
		init_memmap(pmm::pa_to_page(pmm::pavailable_start()), page_count);
	}
	else
	{
		for (int i = reserved_spaces_count - 1; i >= 0; i--)
		{
			if (((size_t)i) == reserved_spaces_count - 1)
			{
				init_memmap(pmm::pa_to_page(reserved_spaces[i].second),
					count_of_pages(reserved_spaces[i].second, max_pa));
			}
			else if (i == 0)
			{
				init_memmap(pmm::pa_to_page(pmm::pavailable_start()),
					count_of_pages(pmm::pavailable_start(), reserved_spaces[i].first));
			}
			else
			{
				init_memmap(pmm::pa_to_page(reserved_spaces[i].second),
					count_of_pages(reserved_spaces[i].second, reserved_spaces[i + 1].first));
			}
		}
	}
}

void pmm::init_pmm()
{

	init_pmm_manager();

	init_physical_mem();

	vmm::install_gdt();

	memory::kmem::kmem_init();

	vmm::init_vmm();

	vmm::boot_map_kernel_mem();

	vmm::install_kernel_pml4t();

}

