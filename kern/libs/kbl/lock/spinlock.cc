#include "arch/amd64/cpu/x86.h"

#include "drivers/apic/traps.h"
#include "drivers/console/console.h"
#include "debug/kdebug.h"
#include "kbl/lock/spinlock.h"

using lock::spinlock_struct;

void lock::spinlock_initialize_lock(spinlock_struct* lk, const char* name)
{
	arch_spinlock_initialize_lock(lk, name);
}

void lock::spinlock_acquire(spinlock_struct* lock)
{
	if (spinlock_holding(lock))
	{
		kdebug::kdebug_dump_lock_panic(lock);
	}

	kdebug::kdebug_get_caller_pcs(16, lock->pcs);

	arch_spinlock_acquire(lock);

	lock->cpu = cpu();

}

void lock::spinlock_release(spinlock_struct* lock)
{
	if (!spinlock_holding(lock))
	{
		KDEBUG_RICHPANIC("Release a not-held spinlock_struct.\n",
			"KERNEL PANIC",
			false,
			"Lock's name: %s", lock->name);
	}

	lock->pcs[0] = 0;
	lock->cpu = nullptr;

	arch_spinlock_release(lock);
}

bool lock::spinlock_holding(spinlock_struct* lock)
{
	return lock->locked && lock->cpu == cpu();
}

void lock::spinlock::lock() noexcept
{
	spinlock_acquire(&this->_spinlock);
}

void lock::spinlock::unlock() noexcept
{
	spinlock_release(&this->_spinlock);
}

bool lock::spinlock::try_lock() noexcept
{
	if (spinlock_holding(&this->_spinlock))
	{
		return false;
	}
	else
	{
		this->lock();
		return true;
	}
}
bool lock::spinlock::holding() noexcept
{
	return spinlock_holding(&this->_spinlock);
}
