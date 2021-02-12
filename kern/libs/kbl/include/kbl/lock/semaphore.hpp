#pragma once

#include "task/thread/thread.hpp"

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

	error_code wait()
	{
		return wait(deadline::infinite());
	}

	error_code wait(const deadline& ddl);

	void signal();

	uint64_t count() const
	{
		ktl::mutex::lock_guard g{ task::global_thread_lock };
		return count_;
	}

	size_t waiter_count() const
	{
		ktl::mutex::lock_guard g{ task::global_thread_lock };
		return wait_queue_.size();
	}

 private:
	task::wait_queue wait_queue_{};
	uint64_t count_{ 0 };
};
}