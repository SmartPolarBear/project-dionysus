
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

page* pmm::pages = nullptr;
size_t pmm::page_count = 0;

constexpr size_t RESERVED_SPACES_MAX_COUNT = 32;
// the count of module tags can't be more than reserved spaces
multiboot::multiboot_tag_ptr module_tags[RESERVED_SPACES_MAX_COUNT];

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

static inline void init_physical_mem()
{
	auto memtag = multiboot::acquire_tag_ptr<multiboot_tag_mmap>(MULTIBOOT_TAG_TYPE_MMAP);
	span<multiboot_mmap_entry> entries{ memtag->entries,
	                                    (memtag->size - sizeof(multiboot_tag_mmap)) / memtag->entry_size };

	uintptr_t max_pa = entries.begin()->addr;

	for (const auto& entry:entries)
	{
		max_pa = max((uintptr_t)(entry.addr + entry.len), max_pa);
	}

	page_count = max_pa / PAGE_SIZE;

	// FIXME: We may not need to do this ?
	// The page management structure is placed a page after kernel
	// So as to protect the multiboot info
	pages = (page*)roundup((uintptr_t)(end + PAGE_SIZE * 2), PAGE_SIZE);

	for (size_t i = 0; i < page_count; i++)
	{
		pages[i].flags |= PHYSICAL_PAGE_FLAG_RESERVED; // set all pages as reserved
	}

	for (const auto& entry:entries)
	{
		if (entry.addr + entry.len < pmm::pavailable_start())
			continue;

		if (entry.type == MULTIBOOT_MEMORY_AVAILABLE)
		{
			physical_memory_manager::instance()->setup_for_base(pmm::pa_to_page(max((uintptr_t)entry.addr,
				pmm::pavailable_start())),
				PAGE_ROUNDDOWN(entry.len) / PAGE_SIZE);
		}
	}

	// reserve all the boot modules
	size_t module_count = multiboot::get_all_tags(MULTIBOOT_TAG_TYPE_MODULE, module_tags, RESERVED_SPACES_MAX_COUNT);
	for (size_t i = 0; i < module_count; i++)
	{
		multiboot_tag_module* tag = reinterpret_cast<decltype(tag)>(module_tags[i]);
		if (tag->mod_end >= pmm::pavailable_start())
		{
			KDEBUG_ASSERT(false);
		}
	}

}

void pmm::init_pmm()
{
	if (!memory::physical_memory_manager::instance()->is_well_constructed())
	{
		KDEBUG_GENERALPANIC("Can't initialize physical_memory_manager");
	}

	init_physical_mem();

	vmm::install_gdt();

	memory::kmem::kmem_init();

	vmm::init_vmm();

	vmm::boot_map_kernel_mem();

	vmm::install_kernel_pml4t();

}

