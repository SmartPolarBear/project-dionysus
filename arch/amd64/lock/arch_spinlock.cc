#include "arch/amd64/cpu/x86.h"

#include "drivers/apic/traps.h"
#include "drivers/console/console.h"
#include "debug/kdebug.h"
#include "drivers/lock/spinlock.h"

using lock::spinlock;

void lock::arch_spinlock_initialize_lock(spinlock* lk, const char* name)
{
	lk->name = name;
	lk->locked = 0u;
	lk->cpu = nullptr;
}

void lock::arch_spinlock_acquire(spinlock* lock)
{
	trap::pushcli();
	if (spinlock_holding(lock))
	{
		kdebug::kdebug_dump_lock_panic(lock);
	}

	while (xchg(&lock->locked, 1u) != 0);

	lock->cpu = cpu();

	kdebug::kdebug_get_caller_pcs(16, lock->pcs);
}

void lock::arch_spinlock_release(spinlock* lock)
{
	if (!spinlock_holding(lock))
	{
		KDEBUG_RICHPANIC("Release a not-held spinlock.\n",
			"KERNEL PANIC",
			false,
			"Lock's name: %s", lock->name);
	}

	lock->pcs[0] = 0;
	lock->cpu = nullptr;

	xchg(&lock->locked, 0u);

	trap::popcli();
}

bool lock::arch_spinlock_holding(spinlock* lock)
{
	return lock->locked && lock->cpu == cpu();
}