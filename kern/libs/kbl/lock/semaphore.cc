#include "kbl/lock/semaphore.hpp"

#include "ktl/mutex/lock_guard.hpp"

void kbl::semaphore::wait()
{
	count_.fetch_sub(1, memory_order_release);
	if (_count_ < 0)
	{
		ktl::mutex::lock_guard g{ task::global_thread_lock };
		wait_queue_.block(task::wait_queue::interruptible::No);
	}
}

void kbl::semaphore::signal()
{
	count_.fetch_add(1, memory_order_release);
	if (_count_ <= 0)
	{
		ktl::mutex::lock_guard g{ task::global_thread_lock };
		wait_queue_.wake_one(true, ERROR_SUCCESS);
	}
}
