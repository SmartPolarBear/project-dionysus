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


