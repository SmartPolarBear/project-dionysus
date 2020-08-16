#include "arch/amd64/x86.h"

#include "drivers/apic/traps.h"
#include "drivers/console/console.h"
#include "drivers/debug/kdebug.h"
#include "drivers/lock/spinlock.h"

using lock::spinlock;

void lock::spinlock_initlock(spinlock* splk, const char* name)
{
	splk->name = name;
	splk->locked = 0u;
	splk->cpu = nullptr;
}

void lock::spinlock_acquire(spinlock* lock)
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

void lock::spinlock_release(spinlock* lock)
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

bool lock::spinlock_holding(spinlock* lock)
{
	return lock->locked && lock->cpu == cpu();
}
