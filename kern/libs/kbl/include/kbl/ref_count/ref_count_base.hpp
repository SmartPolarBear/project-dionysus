#pragma once
#include "kbl/atomic/atomic_ref.hpp"

namespace kbl
{
	template<bool CheckUseAfterFreeOrPreAdopt>
	class ref_count_base
	{
	 public:
		using ref_count_value_type = int64_t;
	 protected:
		constexpr ref_count_base()
			: ref_count_val(PRE_ADOPT_SENTINEL)
		{
		}

		~ref_count_base()
		{
			if (CheckUseAfterFreeOrPreAdopt)
			{
				ref_count.store(PRE_ADOPT_SENTINEL, kbl::memory_order_release);
			}
		}

		void ref_add() const
		{
			const ref_count_value_type rc = ref_count.fetch_add(1, memory_order_relaxed);

			if (CheckUseAfterFreeOrPreAdopt)
			{
				KDEBUG_ASSERT(rc >= 1);
			}
		}

		[[nodiscard]]bool release() const
		{
			const ref_count_value_type rc = ref_count.fetch_sub(1, memory_order_release);
			if (CheckUseAfterFreeOrPreAdopt)
			{
				KDEBUG_ASSERT(rc >= 1);
			}

			if (rc == 1)
			{
				__atomic_thread_fence(int(kbl::memory_order_acquire));
				return true;
			}

			return false;
		}

		void adopt() const
		{
			if (CheckUseAfterFreeOrPreAdopt)
			{
				ref_count_value_type expected = PRE_ADOPT_SENTINEL;
				bool ret = ref_count.compare_exchange_strong(expected, 1, memory_order_acq_rel, memory_order_acquire);
				KDEBUG_ASSERT(ret != 0);
			}
			else
			{
				ref_count.store(1, memory_order_release);
			}
		}

		[[nodiscard]]ref_count_value_type get_ref_count() const
		{
			return ref_count.load(memory_order_relaxed);
		}

		static constexpr ref_count_value_type PRE_ADOPT_SENTINEL = static_cast<ref_count_value_type>(INT64_MIN);

		ref_count_value_type ref_count_val;
		mutable kbl::integral_atomic_ref<ref_count_value_type> ref_count{ ref_count_val };
	};
}