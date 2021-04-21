#include "arch/amd64/cpu/x86.h"

#include "debug/backtrace.hpp"

#include "drivers/apic/traps.h"
#include "drivers/console/console.h"
#include "drivers/acpi/cpu.h"
#include "debug/kdebug.h"
#include "kbl/lock/spinlock.h"

#include "../../libs/basic_io/include/builtin_text_io.hpp"

using lock::spinlock_struct;

[[noreturn]] static inline void dump_lock_panic(lock::arch_spinlock* lock, const char* caller = nullptr)
{
	// disable the lock of console
	console::console_set_lock(false);
	console::console_set_pos(0);

	console::console_set_color(console::CONSOLE_COLOR_BLUE, console::CONSOLE_COLOR_LIGHT_BROWN);

	write_format("[cpu %d]lock %s has been held by cpu %d.\nspinlock value=%lld.\nCall stack of lock:\n",
		cpu->id,
		lock->name,
		arch_spinlock_cpu(lock),
		lock->value);

	if (caller)
	{
		write_format("-> called by %s\n", caller);
	}

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

	// infinite loop
	for (;;);
}

void lock::spinlock_initialize_lock(spinlock_struct* lk, const char* name)
{
	lk->arch.name = name;
	lk->arch.value = 0;
	lk->arch.pcs[0] = 0;
}

void lock::spinlock_acquire(spinlock_struct* lock, bool pres_intr) TA_NO_THREAD_SAFETY_ANALYSIS
{
	if (spinlock_holding(lock))
	{
		dump_lock_panic(&lock->arch, __FUNCTION__);
	}

	kdebug::kdebug_get_backtrace(lock->arch.pcs);

	if (pres_intr)lock->intr = arch_interrupt_save();

	arch_spinlock_lock(&lock->arch);
}

void lock::spinlock_release(spinlock_struct* lock, bool pres_intr) TA_NO_THREAD_SAFETY_ANALYSIS
{
	if (!spinlock_holding(lock))
	{
		KDEBUG_RICHPANIC("Release a not-held spinlock_struct.\n",
			"KERNEL PANIC",
			false,
			"Lock's name_: %s", lock->arch.name);
	}

	lock->arch.pcs[0] = 0;

	arch_spinlock_unlock(&lock->arch);

	if (pres_intr)arch_interrupt_restore(lock->intr);
}

bool lock::spinlock_holding(spinlock_struct* lock) TA_NO_THREAD_SAFETY_ANALYSIS
{
//	return lock->locked && lock->cpu == cpu.get();
	return lock->arch.value != 0 && arch_spinlock_cpu(&lock->arch) == cpu->id;
}

void lock::spinlock::lock() noexcept
{
	state_ = arch_interrupt_save();

	assert_not_held();

	kdebug::kdebug_get_backtrace(spinlock_.pcs);

	arch_spinlock_lock(&spinlock_);
}

void lock::spinlock::unlock() noexcept
{
	assert_held();

	arch_spinlock_unlock(&spinlock_);

	spinlock_.pcs[0] = 0;

	arch_interrupt_restore(state_);
}

bool lock::spinlock::try_lock() noexcept
{
	return !arch_spinlock_try_lock(&spinlock_);
}
bool lock::spinlock::holding() noexcept
{
	if (cpu.is_valid())
	{
//		return spinlock_.locked && spinlock_.cpu == cpu.get();
		return spinlock_.value != 0 && arch_spinlock_cpu(&spinlock_) == cpu->id;
	}
	else
	{
//		return spinlock_.locked;
		return spinlock_.value;
	}
}

bool lock::spinlock::not_holding() noexcept
{
	return !holding();
}

void lock::spinlock::assert_held() TA_ASSERT(this)
{
	if (!holding())
	{
		dump_lock_panic(&spinlock_, __FUNCTION__);
	}
	KDEBUG_ASSERT(holding());
}

void lock::spinlock::assert_not_held() TA_ASSERT(!this)
{
	if (holding())
	{
		dump_lock_panic(&spinlock_, __FUNCTION__);
	}
	KDEBUG_ASSERT(not_holding());
}
