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
	[[nodiscard]] error_code wait() TA_REQ(!task::global_thread_lock);

	/// \brief Try to do a P operation.
	/// \return true if P op is really done.
	[[nodiscard]]bool try_wait() TA_REQ(!task::global_thread_lock);

	/// \brief P operation, or sleep, down
	/// \return
	[[nodiscard]] error_code wait(const deadline& ddl) TA_REQ(!task::global_thread_lock);

	/// \brief V operation, or wakeup, up
	void signal() TA_REQ(!task::global_thread_lock);

	[[nodiscard]] size_t waiter_count() const TA_REQ(!task::global_thread_lock);

	[[nodiscard]] uint64_t count() const
	{
		return count_.load(ktl::memory_order_acquire);
	}

 private:
	/// \brief Try to do a P operation.
	/// \return true if P op is really done.
	[[nodiscard]] bool try_wait_locked() TA_REQ(task::global_thread_lock);

	/// \brief P operation, or sleep, down
	/// \return
	[[nodiscard]] error_code wait_locked(const deadline& ddl) TA_REQ(task::global_thread_lock);

	/// \brief V operation, or wakeup, up
	void signal_locked() TA_REQ(task::global_thread_lock);

	task::wait_queue wait_queue_{};

	ktl::atomic<uint64_t> count_{ 0 };

};
}