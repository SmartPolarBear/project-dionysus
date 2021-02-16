#pragma once

#include "system/types.h"

#include "debug/thread_annotations.hpp"

#include "ktl/string_view.hpp"

struct cpu_struct;

/*
 * architecture-dependent spinlock facility
 *
 * Following symbols should be provided:
 * 1) arch_spinlock, a type for spinlock, which is standard_layout.
 * 2) arch_spinlock_lock, arch_spinlock_try_lock, arch_spinlock_unlock, three basic actions
 * 3) arch_spinlock_cpu, two checking functions
 *
 * Following should be provide as compile-time constant
 * ARCH_SPINLOCK_INITIAL
 *
 */

#pragma push_macro("ENABLE_DEBUG_FACILITY")

namespace lock
{

struct TA_CAP("mutex") arch_spinlock
{
	uint64_t value;

#ifdef _KERNEL_ENABLE_DEBUG_FACILITY
	ktl::string_view name;
	uintptr_t pcs[21];
#endif
};

constexpr arch_spinlock ARCH_SPINLOCK_INITIAL{ 0, ""sv, { 0 }};

void arch_spinlock_lock(arch_spinlock* l) TA_ACQ(l);
void arch_spinlock_unlock(arch_spinlock* l) TA_REL(l);
bool arch_spinlock_try_lock(arch_spinlock* l) TA_TRY_ACQ(false, l);

static inline cpu_num_type arch_spinlock_cpu(const arch_spinlock* lock)
{
	return (cpu_num_type)__atomic_load_n(&lock->value, __ATOMIC_RELAXED) - 1;
}

//void arch_spinlock_acquire(arch_spinlock* lock);
//void arch_spinlock_initialize_lock(arch_spinlock* lk, const char* name);
//void arch_spinlock_release(arch_spinlock* lock);
}
