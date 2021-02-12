#pragma once

//#include "drivers/acpi/cpu.h"
struct cpu_struct;

#include "arch/amd64/lock/arch_spinlock.hpp"

#include "system/types.h"

#include "kbl/lock/lockable.hpp"
#include "kbl/lock/mutex.hpp"

#include "debug/thread_annotations.hpp"

#include <span>

namespace lock
{
using spinlock_struct = arch_spinlock;

void spinlock_acquire(spinlock_struct* lock, bool pres_intr = true);
void spinlock_release(spinlock_struct* lock, bool pres_intr = true);
bool spinlock_holding(spinlock_struct* lock);
void spinlock_initialize_lock(spinlock_struct* lk, const char* name);

class TA_CAP("mutex") spinlock final
	: public mutex
{
 public:
	constexpr spinlock() = default;
	constexpr explicit spinlock(const char* name)
	{
		spinlock_.name = name;
	}

	void lock() noexcept final
	TA_ACQ();

	void unlock() noexcept final
	TA_REL();

	bool try_lock() noexcept final
	TA_TRY_ACQ(true);

	bool holding() noexcept final;

	bool not_holding() noexcept;

	// assertions
	void assert_held()  TA_ASSERT(this);

	void assert_not_held()  TA_ASSERT(!this);


	// for negative capabilities
	const spinlock& operator!() const
	{
		return *this;
	}

 private:
	spinlock_struct spinlock_{};
	bool intr_{ false };
};

} // namespace lock
