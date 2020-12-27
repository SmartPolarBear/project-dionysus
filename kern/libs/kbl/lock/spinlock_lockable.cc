#include "arch/amd64/cpu/x86.h"

#include "drivers/apic/traps.h"
#include "drivers/console/console.h"
#include "debug/kdebug.h"
#include "kbl/lock/spinlock.h"


void lock::spinlock_lockable::lock() noexcept
{
	spinlock_acquire(this->lk);
}

void lock::spinlock_lockable::unlock() noexcept
{
	spinlock_release(this->lk);
}

bool lock::spinlock_lockable::try_lock() noexcept
{
	if (spinlock_holding(this->lk))
	{
		return false;
	}
	spinlock_acquire(this->lk);
	return true;
}
