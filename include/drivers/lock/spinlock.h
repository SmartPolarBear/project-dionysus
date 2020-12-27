#pragma once

//#include "drivers/acpi/cpu.h"
struct cpu_struct;

#include "arch/amd64/lock/arch_spinlock.hpp"

#include "system/types.h"

#include "drivers/lock/lockable.hpp"

#include "debug/thread_annotations.hpp"

namespace lock
{
	using spinlock = arch_spinlock;

	void spinlock_acquire(spinlock* lock);
	bool spinlock_holding(spinlock* lock);
	void spinlock_initialize_lock(spinlock* lk, const char* name);
	void spinlock_release(spinlock* lock);

	class TA_CAP("mutex") spinlock_lockable final
		: public lockable
	{
	 private:
		spinlock* lk;

		spinlock internal_lock{};
	 public:
		spinlock_lockable() = delete;

		[[maybe_unused]] explicit spinlock_lockable(spinlock& _lk) : lk(&_lk)
		{
		}

		void lock() noexcept final
		TA_ACQ();

		void unlock() noexcept final
		TA_TRY_ACQ(false);

		bool try_lock() noexcept final
		TA_REL();
	};

} // namespace lock
