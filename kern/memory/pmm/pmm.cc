#include "memory/pmm.hpp"

#include "arch/amd64/cpu/x86.h"

#include "system/mmu.h"
#include "system/pmm.h"

#include "debug/kdebug.h"

#include "kbl/lock/lock_guard.hpp"

#include <algorithm>
#include <utility>

#include <gsl/util>

using namespace memory;
using namespace lock;

using namespace pmm;

physical_memory_manager* physical_memory_manager::instance()
{
	static physical_memory_manager inst;
	return &inst;
}

memory::physical_memory_manager::physical_memory_manager()
	: provider_()
{
}

void memory::physical_memory_manager::setup_for_base(page* base, size_t n)
{
	return provider_.setup_for_base(base, n);
}

void* physical_memory_manager::asserted_allocate()
{

	// shouldn't have interrupts.
	// popcli&pushcli can neither be used because unprepared cpu local storage
	KDEBUG_ASSERT(!(read_eflags() & EFLAG_IF));

	// we don't reuse alloc_page() because spinlock_struct may not be prepared.
	// call _locked without the lock to avoid double faults when starting APs
	auto page = physical_memory_manager::instance()->allocate_locked(1);

	KDEBUG_ASSERT(page != nullptr);

	return reinterpret_cast<void*>(pmm::page_to_va(page));
}

page* physical_memory_manager::allocate()
{
	return allocate(1);
}

page* memory::physical_memory_manager::allocate(size_t n)
{
	lock_guard g{ lock_ };
	return allocate_locked(n);
}

page* physical_memory_manager::allocate_locked(size_t n)
{
	return provider_.allocate(n);
}

void physical_memory_manager::free(page* base)
{
	free(base, 1);
}

void memory::physical_memory_manager::free(page* base, size_t n)
{
	lock_guard g{ lock_ };
	return provider_.free(base, n);
}

size_t memory::physical_memory_manager::free_count() const
{
	lock_guard g{ lock_ };
	return provider_.free_count();
}

bool memory::physical_memory_manager::is_well_constructed() const
{
	return provider_.is_well_constructed();
}

error_code_with_result<page*> physical_memory_manager::allocate(uintptr_t va,
	size_t n,
	uint64_t perm,
	vmm::pde_ptr_t pgdir,
	bool rewrite_if_exist)
{
	auto pages = physical_memory_manager::instance()->allocate(n);
	if (pages != nullptr)
	{
		page* p = pages;
		size_t va_cnt = 0;
		do
		{
			if (auto ret = insert_page(p, va + va_cnt * PAGE_SIZE, perm, pgdir, rewrite_if_exist);ret != ERROR_SUCCESS)
			{
				if (ret == -ERROR_REWRITE)
				{
					va_cnt++;
				}
				else
				{
					physical_memory_manager::instance()->free(pages, n);
					return ret;
				}
			}
			else
			{
				p++;
				va_cnt++;
			}
		}
		while (p != pages + n);

		if (p != pages + n)
		{
			for (; p != pages + n; p++)
			{
				physical_memory_manager::instance()->free(p);
			}
		}
	}

	return pages;
}
error_code_with_result<page*> physical_memory_manager::allocate(uintptr_t va,
	uint64_t perm,
	vmm::pde_ptr_t pgdir,
	bool rewrite_if_exist)
{
	KDEBUG_ASSERT(pgdir != nullptr);
	KDEBUG_ASSERT(va != 0);

	page* page = memory::physical_memory_manager::instance()->allocate(); //alloc_page();
	if (page != nullptr)
	{
		if (auto ret = insert_page(page, va, perm, pgdir, rewrite_if_exist);ret != ERROR_SUCCESS)
		{
			physical_memory_manager::instance()->free(page);

			return ret;
		}
	}
	return page;
}

error_code physical_memory_manager::insert_page(page* page,
	uintptr_t va,
	uint64_t perm,
	vmm::pde_ptr_t pgdir,
	bool allow_rewrite)
{
	auto pde = vmm::walk_pgdir(pgdir, va, true);
	if (pde == nullptr)
	{
		return -ERROR_MEMORY_ALLOC;
	}

	++page->ref;

	if (*pde != 0)
	{
		if ((!allow_rewrite) && (pde_to_page(pde) != page))
		{
			return -ERROR_REWRITE;
		}

		if ((*pde & PG_P) && pde_to_page(pde) == page)
		{
			--page->ref;
		}
		else
		{
			remove_from_pgdir(pde, pgdir, va);
		}
	}

	*pde = page_to_pa(page) | PG_PS | PG_P | perm;
	memory::physical_memory_manager::instance()->flush_tlb(pgdir, va);

	return ERROR_SUCCESS;
}

void physical_memory_manager::flush_tlb(vmm::pde_ptr_t pgdir, uintptr_t va)
{
	if (rcr3() == V2P((uintptr_t)pgdir))
	{
		invlpg((void*)va);
	}
}

void physical_memory_manager::remove_from_pgdir(vmm::pde_ptr_t pde, vmm::pde_ptr_t pgdir, uintptr_t va)
{
	if ((*pde) & PG_P)
	{
		auto page = pmm::pde_to_page(pde);
		if ((--page->ref) == 0)
		{
			physical_memory_manager::instance()->free(page);
		}
		*pde = 0;

		memory::physical_memory_manager::instance()->flush_tlb(pgdir, va);
	}
}

void physical_memory_manager::remove_page(uintptr_t va, vmm::pde_ptr_t pgdir)
{
	auto pde = vmm::walk_pgdir(pgdir, va, false);
	if (pde != nullptr)
	{
		remove_from_pgdir(pde, pgdir, va);
	}
}


