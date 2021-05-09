#include "include/pmm.h"
#include "include/buddy_pmm.h"

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

#include "../../libs/basic_io/include/builtin_text_io.hpp"

#include "kbl/lock/lock_guard.hpp"

#include <algorithm>
#include <utility>

#include <gsl/util>

memory::physical_memory_manager::physical_memory_manager()
	: provider_()
{
}

void memory::physical_memory_manager::setup_for_base(page_info* base, size_t n)
{
	return provider_.setup_for_base(base, n);
}

page_info* memory::physical_memory_manager::allocate(size_t n)
{
	return provider_.allocate(n);
}

void memory::physical_memory_manager::free(page_info* base, size_t n)
{
	return provider_.free(base, n);
}

size_t memory::physical_memory_manager::free_count() const
{
	return provider_.free_count();
}
bool memory::physical_memory_manager::is_well_constructed() const
{
	return provider_.is_well_constructed();
}
