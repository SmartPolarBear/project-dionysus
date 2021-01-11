#pragma once

#include "kbl/atomic/atomic_ref.hpp"

namespace kbl
{
template<typename T>
class integral_atomic
{
 public:
	// integral_atomic_ref is only implemented for integral types, which is a stronger requirement than
	// std's, which only requires T be trivially copyable.
	static_assert(std::is_integral_v<T>);
	using value_type = T;
	using difference_type = value_type;

	static constexpr bool is_always_lock_free = __atomic_always_lock_free(sizeof(T), nullptr);

	integral_atomic() = default;

	explicit integral_atomic(T& obj) : value(obj)
	{
	}

	integral_atomic(const integral_atomic&) noexcept = default;
	integral_atomic(integral_atomic&&) noexcept = default;

	T operator=(T desired) const noexcept
	{
		__atomic_store_n(ptr_, desired, __ATOMIC_SEQ_CST);
		return desired;
	}
	integral_atomic& operator=(const integral_atomic&) = delete;

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
	T value;
	T* const ptr_{ &value };
};

void atomic_thread_fence(kbl::memory_order_type __m) noexcept
{
	__atomic_thread_fence(int(__m));
}

void atomic_signal_fence(kbl::memory_order_type __m) noexcept
{
	__atomic_signal_fence(int(__m));
}

}