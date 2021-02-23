#pragma once
#include "debug/kdebug.h"

class ref_counted
{
 protected:
	constexpr ref_counted() : ref_count_(PRE_ADOPT_SENTINEL)
	{
	}

	~ref_counted()
	{
		ref_count_.store(PRE_ADOPT_SENTINEL, ktl::memory_order_release);
	}

	void add_ref() const
	{
		auto rc = ref_count_.fetch_and(1, ktl::memory_order_release);
		KDEBUG_ASSERT(rc >= 1);
	}

	[[nodiscard]]bool release() const
	{
		auto rc = ref_count_.fetch_sub(1, ktl::memory_order_release);
		KDEBUG_ASSERT(rc >= 1);

		if (rc == 1)
		{
			ktl::atomic_thread_fence(std::memory_order_acquire);
			return true;
		}

		return false;
	}

	void adopt() const
	{
		auto expected = PRE_ADOPT_SENTINEL;
		KDEBUG_ASSERT(ref_count_.compare_exchange_strong(expected,
			1,
			std::memory_order_acq_rel,
			std::memory_order_acquire));
	}

	auto ref_count() const
	{
		return ref_count_.load(std::memory_order_relaxed);
	}

	mutable ktl::atomic<int64_t> ref_count_{ PRE_ADOPT_SENTINEL };

 private:
	static constexpr int64_t PRE_ADOPT_SENTINEL{ 0xC0000000 };
};