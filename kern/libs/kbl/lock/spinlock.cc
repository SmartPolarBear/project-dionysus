#include "arch/amd64/cpu/x86.h"

#include "debug/backtrace.hpp"

#include "drivers/apic/traps.h"
#include "drivers/console/console.h"
#include "drivers/acpi/cpu.h"
#include "debug/kdebug.h"
#include "kbl/lock/spinlock.h"

#include "../../libs/basic_io/include/builtin_text_io.hpp"

using lock::spinlock_struct;

[[noreturn]] static inline void dump_lock_panic(lock::spinlock_struct* lock)
{
	// disable the lock of console
	console::console_set_lock(false);
	console::console_set_pos(0);

	console::console_set_color(console::CONSOLE_COLOR_BLUE, console::CONSOLE_COLOR_LIGHT_BROWN);

	write_format("[cpu %d]lock %s has been held.\nCall stack of lock:\n", cpu->id, lock->name);

	{
		size_t counter = 0;
		for (auto cs : lock->pcs)
		{
			if (cs == 0)break;
			write_format("%p ", cs);
			if ((++counter) % 4 == 0)write_format("\n");
		}
	}

	write_format("\nCall stack of panic:\n");

	kdebug::kdebug_print_backtrace();

	// set global panic state for other cpu
	kdebug::panicked = true;

	// infinite loop
	for (;;);
}

void lock::spinlock_initialize_lock(spinlock_struct* lk, const char* name)
{
	arch_spinlock_initialize_lock(lk, name);
}

void lock::spinlock_acquire(spinlock_struct* lock, bool pres_intr)
{
	if (spinlock_holding(lock))
	{
		dump_lock_panic(lock);
	}

	kdebug::kdebug_get_backtrace(lock->pcs);

	if (pres_intr)trap::pushcli();
	arch_spinlock_acquire(lock);

	lock->cpu = cpu.get();

}

void lock::spinlock_release(spinlock_struct* lock, bool pres_intr)
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

	if (pres_intr)trap::popcli();
}

bool lock::spinlock_holding(spinlock_struct* lock)
{
	return lock->locked && lock->cpu == cpu.get();
}

void lock::spinlock::lock() noexcept
{
	if (holding())
	{
		dump_lock_panic(&spinlock_);
	}

	kdebug::kdebug_get_backtrace(spinlock_.pcs);

	// hack: cpu.get_use_lock() is true after it become usable
	intr_ = !arch_ints_disabled();
	if (intr_)cli();

	arch_spinlock_acquire(&spinlock_);
}

void lock::spinlock::unlock() noexcept
{
	if (!holding())
	{
		KDEBUG_RICHPANIC("Release a not-held spinlock_struct.\n",
			"KERNEL PANIC",
			false,
			"Lock's name: %s", spinlock_.name);
	}

	memset(spinlock_.pcs, 0, sizeof(spinlock_.pcs));

	arch_spinlock_release(&spinlock_);

	if (intr_)sti();
	intr_ = false;
}

bool lock::spinlock::try_lock() noexcept
{
	if (spinlock_.locked)
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
	if (cpu.get_use_lock())
	{
		return spinlock_.locked && spinlock_.cpu == cpu.get();
	}
	else
	{
		return spinlock_.locked;
	}
}
