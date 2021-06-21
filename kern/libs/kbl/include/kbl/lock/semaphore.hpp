#pragma once

#include "debug/thread_annotations.hpp"

#include "task/thread/wait_queue.hpp"

#include "ktl/atomic.hpp"

namespace kbl
{

class semaphore final
{
 public:
	[[nodiscard]] semaphore() = default;
	explicit semaphore(uint64_t init_count) : count_(init_count)
	{
	}
	~semaphore() = default;

	semaphore(const semaphore&) = delete;
	semaphore& operator=(const semaphore&) = delete;
	semaphore(const semaphore&&) = delete;

	/// \brief P operation, or sleep, down
	/// \return
	[[nodiscard]] error_code wait_locked() TA_REQ(task::global_thread_lock)
	{
		return wait_locked(deadline::infinite());
	}

	/// \brief P operation, or sleep, down
	/// \return
	[[nodiscard]] error_code wait_locked(const deadline& ddl) TA_REQ(task::global_thread_lock);

	/// \brief P operation, or sleep, down
	/// \return
	[[nodiscard]] error_code wait() TA_REQ(!task::global_thread_lock)
	{
		lock::lock_guard g{ task::global_thread_lock };
		return wait_locked();
	}

	/// \brief P operation, or sleep, down
	/// \return
	[[nodiscard]] error_code wait(const deadline& ddl) TA_REQ(!task::global_thread_lock)
	{
		lock::lock_guard g{ task::global_thread_lock };
		return wait_locked(ddl);
	}

	/// \brief V operation, or wakeup, up
	void signal_locked() TA_REQ(task::global_thread_lock);

	/// \brief V operation, or wakeup, up
	void signal() TA_REQ(!task::global_thread_lock)
	{
		lock::lock_guard g{ task::global_thread_lock };
		signal_locked();
	}

	[[nodiscard]] uint64_t count() const
	{
		return count_.load(ktl::memory_order_acquire);
	}

	[[nodiscard]] size_t waiter_count() const TA_REQ(!task::global_thread_lock);

	[[nodiscard]] size_t waiter_count_locked() const TA_REQ(task::global_thread_lock)
	{
		return wait_queue_.size();
	}

 private:
	task::wait_queue wait_queue_{};

	ktl::atomic<uint64_t> count_{ 0 };

};
}