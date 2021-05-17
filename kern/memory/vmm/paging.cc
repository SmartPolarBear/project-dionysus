
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

#include "vmm.h"

#include "system/error.hpp"
#include "system/kmalloc.hpp"
#include "system/memlayout.h"
#include "system/mmu.h"
#include "system/pmm.h"
#include "system/vmm.h"

#include "arch/amd64/cpu/x86.h"

#include "drivers/acpi/cpu.h"
#include "drivers/apic/apic.h"
#include "drivers/apic/traps.h"
#include "drivers/console/console.h"
#include "debug/kdebug.h"

#include "memory/pmm.hpp"

#include "ktl/span.hpp"

#include "../../libs/basic_io/include/builtin_text_io.hpp"
#include <cstring>
#include <algorithm>

using namespace ktl;

using vmm::mm_struct;
using vmm::pde_ptr_t;
using vmm::pde_t;
using vmm::vma_struct;

// linked list
using kbl::list_add;
using kbl::list_empty;
using kbl::list_init;
using kbl::list_remove;

// global variable for the sake of access and dynamically mapping
pde_ptr_t vmm::g_kpml4t;

// remove all flags_ in entries to expose the address of the next level
static inline constexpr size_t remove_flags(vmm::pde_t pde)
{
	constexpr size_t FLAGS_SHIFT = 8;
	return (pde >> FLAGS_SHIFT) << FLAGS_SHIFT;
}

static inline void do_free_range_pgdir(pde_ptr_t pdpte)
{
	auto pgdir = reinterpret_cast<decltype(pdpte)>(P2V(remove_flags(*pdpte)));

	*pgdir = 0;
	vmm::pgdir_entry_free(pgdir);
}

static inline void do_free_range_pdpt(pde_ptr_t pml4e, uintptr_t st, uintptr_t ed)
{
	auto pdpt = reinterpret_cast<decltype(pml4e)>(P2V(remove_flags(*pml4e)));

	for (uintptr_t addr = st; addr <= ed; addr += PDPT_SIZE)
	{
		auto pdpte = &pdpt[P3X(addr)];
		do_free_range_pgdir(pdpte);

		*pdpte = 0;
		vmm::pgdir_entry_free(pdpte);
	}
}

static inline void do_free_range_pml4t(pde_ptr_t pml4t, uintptr_t st, uintptr_t ed)
{
// traversal all the pml4e concerned
	for (uintptr_t addr = st; addr <= ed; addr += PML4T_SIZE)
	{
		pde_ptr_t pml4e = &pml4t[P4X(addr)];
		if ((*pml4e) & PG_P)
		{
			do_free_range_pdpt(pml4e, st, ed);

			*pml4e = 0;
			vmm::pgdir_entry_free(pml4e);
		}
	}
}

// find the pde corresponding to the given va
// perm is only valid when create_if_not_exist = true
static inline pde_ptr_t walk_pgdir(const pde_ptr_t pml4t,
	uintptr_t vaddr,
	bool create_if_not_exist = false,
	size_t perm = 0)
{
	// #define WALK_PGDIR_PRINT_INTERMEDIATE_VAL

	// the pml4, which's the content of CR3, is held in kpml4t
	// firstly find the 3rd page directory (PDPT) from it.
	pde_ptr_t pml4e = &pml4t[P4X(vaddr)],
		pdpt = nullptr,
		pdpte = nullptr,
		pgdir = nullptr,
		pde = nullptr;

#ifdef WALK_PGDIR_PRINT_INTERMEDIATE_VAL
	if (vaddr == 0x10000000)
	{
		printf("pml4e=0x%p\n", pml4e);
	}
#endif

	if (!(*pml4e & PG_P))
	{
		if (!create_if_not_exist)
		{
			return nullptr;
		}

		pdpt = vmm::pgdir_entry_alloc();
		KDEBUG_ASSERT(pdpt != nullptr);
		if (pdpt == nullptr)
		{
			return nullptr;
		}

		memset(pdpt, 0, PGTABLE_SIZE);
		*pml4e = ((V2P((uintptr_t)pdpt)) | PG_P | PG_U | perm);
	}
	else
	{
		pdpt = reinterpret_cast<decltype(pdpt)>(P2V(remove_flags(*pml4e)));
	}

#ifdef WALK_PGDIR_PRINT_INTERMEDIATE_VAL
	if (vaddr == 0x10000000)
	{
		printf("pdpt=0x%p\n", pdpt);
	}
#endif

	// find the 2nd page directory from PGPT
	pdpte = &pdpt[P3X(vaddr)];
	if (!(*pdpte & PG_P))
	{
		if (!create_if_not_exist)
		{
			return nullptr;
		}

		pgdir = vmm::pgdir_entry_alloc();

		KDEBUG_ASSERT(pgdir != nullptr);
		if (pgdir == nullptr)
		{
			return nullptr;
		}

		memset(pgdir, 0, PGTABLE_SIZE);
		*pdpte = ((V2P((uintptr_t)pgdir)) | PG_P | PG_U | perm);
	}
	else
	{
		pgdir = reinterpret_cast<decltype(pgdir)>(P2V(remove_flags(*pdpte)));
	}

#ifdef WALK_PGDIR_PRINT_INTERMEDIATE_VAL
	if (vaddr == 0x10000000)
	{
		printf("pdpte=0x%p\n", pdpte);
	}
#endif

	// find the page from 2nd page directory.
	// because we use page size extension (2mb pages)
	// this is the last level.
	pde = &pgdir[P2X(vaddr)];

#ifdef WALK_PGDIR_PRINT_INTERMEDIATE_VAL
	if (vaddr == 0x10000000)
	{
		printf("pde=0x%p\n", pde);
	}
#endif

	return pde;
}

// this method maps the specific va
static inline error_code map_page(pde_ptr_t pml4, uintptr_t va, uintptr_t pa, size_t perm)
{
	auto pde = walk_pgdir(pml4, va, true, perm);

	if (pde == nullptr)
	{
		return -ERROR_MEMORY_ALLOC;
	}

	if (!(*pde & PG_P))
	{
		*pde = ((pa) | PG_PS | PG_P | PG_U | perm);
	}
	else
	{
		return -ERROR_REWRITE;
	}

	return ERROR_SUCCESS;
}

static inline error_code map_pages(pde_ptr_t pml4,
	uintptr_t va_start,
	uintptr_t pa_start,
	uintptr_t pa_end,
	uint64_t perm)
{
	error_code ret = ERROR_SUCCESS;

	// map the kernel memory
	for (uintptr_t pa = pa_start, va = va_start;
	     pa < pa_end && pa + PAGE_SIZE <= pa_end;
	     pa += PAGE_SIZE, va += PAGE_SIZE)
	{
		ret = map_page(pml4, va, pa, /*PG_W | PG_U*/ perm);

		if (ret != -ERROR_SUCCESS)
		{
			return ret;
		}
	}

	return ret;
}

void vmm::unmap_range(pde_ptr_t pgdir, uintptr_t start, uintptr_t end)
{
	KDEBUG_ASSERT(start % PAGE_SIZE == 0 && end % PAGE_SIZE == 0);
	KDEBUG_ASSERT(VALID_USER_REGION(start, end));

	for (uintptr_t addr = start; addr <= end; addr += PAGE_SIZE)
	{
		auto pte = walk_pgdir(pgdir, addr, false);

		if ((*pte) & PG_P)
		{
			*pte = 0;
			memory::physical_memory_manager::instance()->flush_tlb(pgdir, addr);
		}
	}
}

// the range must be unmapped
void vmm::free_range(pde_ptr_t pml4t, uintptr_t start, uintptr_t end)
{
	KDEBUG_ASSERT(start % PAGE_SIZE == 0 && end % PAGE_SIZE == 0);
	KDEBUG_ASSERT(VALID_USER_REGION(start, end));

	do_free_range_pml4t(pml4t, start, end);
}

void vmm::copy_range(pde_ptr_t from, pde_ptr_t to, uintptr_t start, uintptr_t end)
{
	for (uintptr_t addr = start; addr <= end; addr += PAGE_SIZE)
	{
		auto pte = walk_pgdir(from, addr, false);

		if ((*pte) & PG_P)
		{
			auto perm = *pte & (PG_P | PG_W | PG_U);

			memory::physical_memory_manager::instance()->insert_page(pmm::pde_to_page(pte), addr, perm, to, true);
//			pmm::page_insert(to, true, pmm::pde_to_page(pte), addr, perm);
		}
	}
}

error_code vmm::map_range(pde_ptr_t pgdir, uintptr_t va_start, uintptr_t pa_start, size_t len)
{
	va_start = PAGE_ROUNDDOWN(va_start);
	pa_start = PAGE_ROUNDDOWN(pa_start);
	uintptr_t pa_end = PAGE_ROUNDUP(pa_start + len);
	return map_pages(pgdir, va_start, pa_start, pa_end, PG_W | PG_U);
}

void vmm::install_kernel_pml4t()
{
	lcr3(V2P((uintptr_t)g_kpml4t));
}

void vmm::duplicate_kernel_pml4t(OUT pde_ptr_t pml4t)
{
	memmove(pml4t, g_kpml4t, PGTABLE_SIZE);

	//#define VERIFY_COPY
#ifdef VERIFY_COPY
	KDEBUG_ASSERT(memcmp(pml4t,g_kpml4t,PGTABLE_SIZE)==0);
#endif
}

uintptr_t vmm::pde_to_pa(pde_ptr_t pde)
{
	constexpr size_t FLAGS_SHIFT = 8;
	return ((((*pde) >> FLAGS_SHIFT) << FLAGS_SHIFT) & (~PG_PS));
}

pde_ptr_t vmm::walk_pgdir(pde_ptr_t pgdir, size_t va, bool create)
{
	return ::walk_pgdir(pgdir, va, create, PG_U | PG_W);
}

// When called by pmm, first map [0,2GiB] to [KERNEL_VIRTUALBASE,KERNEL_VIRTUALEND]
// and then map all the memories to PHYREMAP_VIRTUALBASE
void vmm::paging_init()
{
	constexpr auto DEFAULT_PERM = PG_W | PG_U;
	// map the kernel memory: [0,2GiB] to [KERNEL_VIRTUALBASE,KERNEL_VIRTUALEND]
	if (auto ret = map_pages(g_kpml4t, KERNEL_VIRTUALBASE, 0, KERNEL_SIZE, DEFAULT_PERM);ret == -ERROR_MEMORY_ALLOC)
	{
		KDEBUG_GENERALPANIC("Can't allocate enough space for paging.\n");
	}
	else if (ret == -ERROR_REWRITE)
	{
		KDEBUG_RICHPANIC("Remap a mapped page.", "KERNEL PANIC:ERROR_REMAP",
			true, "");
	}

	auto memtag = multiboot::acquire_tag_ptr<multiboot_tag_mmap>(MULTIBOOT_TAG_TYPE_MMAP);
	span<multiboot_mmap_entry> entries{ memtag->entries,
	                                    (memtag->size - sizeof(multiboot_tag_mmap)) / memtag->entry_size };

	KDEBUG_ASSERT(is_sorted(entries.begin(), entries.end(), [](const auto& a, const auto& b)
	{
	  return a.addr < b.addr;
	}));

	auto max_pa = 0ull;
	for (const auto& entry:entries)
	{
		max_pa = std::max(max_pa, std::min(entry.addr + entry.len, (unsigned long long)PHYMEMORY_SIZE));
	}

	// remap all the physical memory
	if (auto ret = map_pages(g_kpml4t, PHYREMAP_VIRTUALBASE, 0, max_pa, DEFAULT_PERM); ret == -ERROR_MEMORY_ALLOC)
	{
		KDEBUG_GENERALPANIC("Can't allocate enough space for paging.\n");
	}
	else if (ret == -ERROR_REWRITE)
	{
		KDEBUG_RICHPANIC("Remap a mapped page.", "KERNEL PANIC:ERROR_REMAP",
			true, "");
	}

	for (const auto& entry:entries)
	{
		// By default, we treat all reserved space as device memory, by setting specified flages on page

		if (entry.type != MULTIBOOT_MEMORY_AVAILABLE)
		{
			auto perm = DEFAULT_PERM | PG_PWT | PG_PCD;
			auto vaddr = PAGE_ROUNDDOWN(P2V_PHYREMAP(entry.addr)),
				vend = PAGE_ROUNDDOWN(P2V_PHYREMAP(entry.addr + entry.len));
			for (uintptr_t addr = vaddr; addr <= vend; addr += PAGE_SIZE)
			{
				auto pte = walk_pgdir(g_kpml4t, addr, false);
				*pte |= perm;
			}
		}
	}

}
