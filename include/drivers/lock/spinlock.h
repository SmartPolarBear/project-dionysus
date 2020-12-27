#pragma once

//#include "drivers/acpi/cpu.h"
struct cpu_struct;

#include "system/types.h"

#include "drivers/lock/lockable.hpp"

#include "debug/thread_annotations.hpp"

namespace lock
{
	struct spinlock
	{
		uint32_t locked;

		const char* name;
		cpu_struct* cpu;
		uintptr_t pcs[16];
	};

	void spinlock_acquire(spinlock* lock);
	bool spinlock_holding(spinlock* lock);
	void spinlock_initialize_lock(spinlock* lk, const char* name);
	void spinlock_release(spinlock* lock);

	class spinlock_lockable final
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

		void lock() noexcept final ;
		void unlock() noexcept final;
		bool try_lock() noexcept final;
	}TA_CAP("mutex");

} // namespace lock
