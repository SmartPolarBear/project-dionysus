#pragma once

#include "../../../../../../include/system/types.h"

namespace lock
{
	struct arch_spinlock
	{
		uint32_t locked;

		const char* name;
		cpu_struct* cpu;
		uintptr_t pcs[16];
	};

	void arch_spinlock_acquire(arch_spinlock* lock);
	bool arch_spinlock_holding(arch_spinlock* lock);
	void arch_spinlock_initialize_lock(arch_spinlock* lk, const char* name);
	void arch_spinlock_release(arch_spinlock* lock);
}