#include "kbl/lock/semaphore.hpp"

#include "kbl/lock/lock_guard.hpp"

using namespace task;
error_code kbl::semaphore::wait_locked(const deadline& ddl)
{
//	lock::lock_guard g{ task::global_thread_lock };
	global_thread_lock.assert_held();
	KDEBUG_ASSERT(count_ == 0 || wait_queue_.empty());

	if (count_ > 0)
	{
		--count_;
		return ERROR_SUCCESS;
	}

	return wait_queue_.block(task::wait_queue::interruptible::Yes, ddl);
}

void kbl::semaphore::signal_locked()
{
//	lock::lock_guard g{ task::global_thread_lock };
	global_thread_lock.assert_held();

	KDEBUG_ASSERT(count_ == 0 || wait_queue_.empty());

	if (wait_queue_.empty())
	{
		++count_;
	}
	else
	{
		wait_queue_.wake_one(true, ERROR_SUCCESS);
	}
}
