#include "arch/amd64/x86.h"

#include "drivers/apic/traps.h"
#include "drivers/console/console.h"
#include "debug/kdebug.h"
#include "drivers/lock/spinlock.h"

using lock::spinlock;

void lock::spinlock_initialize_lock(spinlock* lk, const char* name)
{
	arch_spinlock_initialize_lock(lk, name);
}

void lock::spinlock_acquire(spinlock* lock)
{
	arch_spinlock_acquire(lock);
}

void lock::spinlock_release(spinlock* lock)
{
	arch_spinlock_release(lock);
}

bool lock::spinlock_holding(spinlock* lock)
{
	return arch_spinlock_holding(lock);
}
