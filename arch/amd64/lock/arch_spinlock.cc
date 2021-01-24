#include "arch/amd64/cpu/x86.h"
#include "arch/amd64/lock/arch_spinlock.hpp"

#include "drivers/console/console.h"

#define  ARCH_SPINLOCK
#include "drivers/apic/traps.h"

void lock::arch_spinlock_initialize_lock(arch_spinlock* lk, const char* name)
{
	lk->name = name;
	lk->locked = 0u;
	lk->cpu = nullptr;
}

void lock::arch_spinlock_acquire(arch_spinlock* lock)
{
	__sync_synchronize();

	while (xchg(&lock->locked, 1u) != 0);
}

void lock::arch_spinlock_release(arch_spinlock* lock)
{
	__sync_synchronize();
	xchg(&lock->locked, 0u);
}
