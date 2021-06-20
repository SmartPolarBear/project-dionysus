#pragma once

#include "task/thread/wait_queue.hpp"

#include "ktl/atomic.hpp"

namespace kbl
{

class semaphore final
{
 public:
	semaphore() = default;
	explicit semaphore(uint64_t init_count) : count_(init_count)
	{
	}
	~semaphore() = default;

	semaphore(const semaphore&) = delete;
	semaphore& operator=(const semaphore&) = delete;
	semaphore(const semaphore&&) = delete;

	/// \brief P operation, or sleep, down
	/// \return
	error_code wait()
	{
		return wait(deadline::infinite());
	}

	/// \brief P operation, or sleep, down
	/// \return
	error_code wait(const deadline& ddl);

	/// \brief V operation, or wakeup, up
	void signal();

	uint64_t count() const
	{
		lock::lock_guard g{ lock_ };
		return count_.load(ktl::memory_order_acquire);
	}

	size_t waiter_count() const
	{
		lock::lock_guard g{ task::global_thread_lock };

		return wait_queue_.size();
	}

 private:
	task::wait_queue wait_queue_{};

	ktl::atomic<uint64_t> count_{ 0 };

	mutable lock::spinlock lock_{ "semaphore" };
};
}