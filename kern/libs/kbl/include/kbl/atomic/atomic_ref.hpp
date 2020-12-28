#pragma once

namespace kbl
{
// integral_atomic_ref wraps an underlying object and allows atomic operations on the
// underlying object.
//
// kbl::integral_atomic_ref is a subset of std::integral_atomic_ref, until that is available.
// kbl::integral_atomic_ref is only implemented for integral types at this time and
// does not implement wait() / notify_*()

// integral_atomic_ref is useful when dealing with ABI types or when interacting with
// types that are fixed for external reasons; in all other cases, you prefer
// atomic<T>.

	using memory_order_type = size_t;

	constexpr memory_order_type memory_order_relaxed = __ATOMIC_RELAXED;
	constexpr memory_order_type memory_order_consume = __ATOMIC_CONSUME;
	constexpr memory_order_type memory_order_acquire = __ATOMIC_ACQUIRE;
	constexpr memory_order_type memory_order_release = __ATOMIC_RELEASE;
	constexpr memory_order_type memory_order_acq_rel = __ATOMIC_ACQ_REL;
	constexpr memory_order_type memory_order_seq_cst = __ATOMIC_SEQ_CST;

	template<typename T>
	class integral_atomic_ref
	{
	 public:
		// integral_atomic_ref is only implemented for integral types, which is a stronger requirement than
		// std's, which only requires T be trivially copyable.
		static_assert(std::is_integral_v<T>);
		using value_type = T;
		using difference_type = value_type;

		static constexpr bool is_always_lock_free = __atomic_always_lock_free(sizeof(T), nullptr);

		integral_atomic_ref() = delete;
		explicit integral_atomic_ref(T& obj) : ptr_(&obj)
		{
		}
		integral_atomic_ref(const integral_atomic_ref&) noexcept = default;

		T operator=(T desired) const noexcept
		{
			__atomic_store_n(ptr_, desired, __ATOMIC_SEQ_CST);
			return desired;
		}
		integral_atomic_ref& operator=(const integral_atomic_ref&) = delete;

		bool is_lock_free() const noexcept
		{
			return __atomic_always_lock_free(sizeof(T), nullptr);
		}
		void store(T desired, kbl::memory_order_type order = kbl::memory_order_seq_cst) const noexcept
		{
			__atomic_store_n(ptr_, desired, static_cast<int>(order));
		}
		T load(kbl::memory_order_type order = kbl::memory_order_seq_cst) const noexcept
		{
			return __atomic_load_n(ptr_, static_cast<int>(order));
		}
		T exchange(T desired, kbl::memory_order_type order = kbl::memory_order_seq_cst) const noexcept
		{
			return __atomic_exchange_n(ptr_, desired, static_cast<int>(order));
		}
		bool compare_exchange_weak(T& expected, T desired, kbl::memory_order_type success,
			kbl::memory_order_type failure) const noexcept
		{
			return __atomic_compare_exchange_n(ptr_, &expected, desired, true,
				static_cast<int>(success), static_cast<int>(failure));
		}
		bool compare_exchange_weak(T& expected, T desired,
			kbl::memory_order_type order = kbl::memory_order_seq_cst) const noexcept
		{
			return __atomic_compare_exchange_n(ptr_, &expected, desired, true,
				static_cast<int>(order), static_cast<int>(order));
		}
		bool compare_exchange_strong(T& expected, T desired, kbl::memory_order_type success,
			kbl::memory_order_type failure) const noexcept
		{
			return __atomic_compare_exchange_n(ptr_, &expected, desired, false,
				static_cast<int>(success), static_cast<int>(failure));
		}
		bool compare_exchange_strong(T& expected, T desired,
			kbl::memory_order_type order = kbl::memory_order_seq_cst) const noexcept
		{
			return __atomic_compare_exchange_n(ptr_, &expected, desired, false,
				static_cast<int>(order), static_cast<int>(order));
		}

		T fetch_add(T arg, kbl::memory_order_type order = kbl::memory_order_seq_cst) const noexcept
		{
			return __atomic_fetch_add(ptr_, arg, static_cast<int>(order));
		}
		T fetch_sub(T arg, kbl::memory_order_type order = kbl::memory_order_seq_cst) const noexcept
		{
			return __atomic_fetch_sub(ptr_, arg, static_cast<int>(order));
		}
		T fetch_and(T arg, kbl::memory_order_type order = kbl::memory_order_seq_cst) const noexcept
		{
			return __atomic_fetch_and(ptr_, arg, static_cast<int>(order));
		}
		T fetch_or(T arg, kbl::memory_order_type order = kbl::memory_order_seq_cst) const noexcept
		{
			return __atomic_fetch_or(ptr_, arg, static_cast<int>(order));
		}
		T fetch_xor(T arg, kbl::memory_order_type order = kbl::memory_order_seq_cst) const noexcept
		{
			return __atomic_fetch_xor(ptr_, arg, static_cast<int>(order));
		}
		T operator++() const noexcept
		{
			return fetch_add(1) + 1;
		}
		T operator++(int) const noexcept
		{
			return fetch_add(1);
		}
		T operator--() const noexcept
		{
			return fetch_sub(1) - 1;
		}
		T operator--(int) const noexcept
		{
			return fetch_sub(1);
		}
		T operator+=(T arg) const noexcept
		{
			return fetch_add(arg) + arg;
		}
		T operator-=(T arg) const noexcept
		{
			return fetch_sub(arg) - arg;
		}
		T operator&=(T arg) const noexcept
		{
			return fetch_and(arg) & arg;
		}
		T operator|=(T arg) const noexcept
		{
			return fetch_or(arg) | arg;
		}
		T operator^=(T arg) const noexcept
		{
			return fetch_xor(arg) ^ arg;
		}

	 private:
		T* const ptr_;
	};
}