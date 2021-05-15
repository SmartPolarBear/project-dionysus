#pragma once

//#include "drivers/acpi/cpu.h"
#include "arch/amd64/cpu/interrupt.h"
struct cpu_struct;

#include "arch/amd64/lock/arch_spinlock.hpp"

#include "system/types.h"

#include "kbl/lock/lockable.hpp"
#include "kbl/lock/mutex.hpp"

#include "debug/thread_annotations.hpp"

#include <span>

namespace lock
{

struct spinlock_struct
{
	arch_spinlock arch;
	interrupt_saved_state_type intr;
};

void spinlock_acquire(spinlock_struct* lock, bool pres_intr = true);
void spinlock_release(spinlock_struct* lock, bool pres_intr = true);
bool spinlock_holding(spinlock_struct* lock);
void spinlock_initialize_lock(spinlock_struct* lk, const char* name);

class TA_CAP("mutex") spinlock final
{
 public:
	constexpr spinlock() = default;

	constexpr explicit spinlock(const char* name)
	{
		spinlock_.name = name;
	}

	void lock() noexcept
	TA_ACQ();

	void unlock() noexcept
	TA_REL();

	bool try_lock() noexcept
	TA_TRY_ACQ(true);

	// assertions
	void assert_held()  TA_ASSERT(this);

	void assert_not_held()  TA_ASSERT(!this);

	// for negative capabilities
	const spinlock& operator!() const
	{
		return *this;
	}

	bool holding() noexcept;

	bool not_holding() noexcept;

 private:
	arch_spinlock spinlock_{};
	interrupt_saved_state_type state_{ 0 };
};
static_assert(ktl::is_standard_layout_v<spinlock>);
static_assert(Mutex<spinlock>, "Spinlock should satisfy the requirement of Mutex");

} // namespace lock
