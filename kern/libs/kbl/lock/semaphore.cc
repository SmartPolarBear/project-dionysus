#include "kbl/lock/semaphore.hpp"

#include "kbl/lock/lock_guard.hpp"

using namespace task;

bool kbl::semaphore::try_wait()
{
	lock::lock_guard g{ task::global_thread_lock };
	return try_wait_locked();
}

error_code kbl::semaphore::wait_locked(const deadline& ddl)
{
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
size_t kbl::semaphore::waiter_count() const TA_REQ(!task::global_thread_lock)
{
	lock::lock_guard g{ task::global_thread_lock };
	return wait_queue_.size();
}

error_code kbl::semaphore::wait() TA_REQ(!task::global_thread_lock)
{
	lock::lock_guard g{ task::global_thread_lock };
	return wait_locked(deadline::infinite());
}

error_code kbl::semaphore::wait(const deadline& ddl) TA_REQ(!task::global_thread_lock)
{
	lock::lock_guard g{ task::global_thread_lock };
	return wait_locked(ddl);
}

void kbl::semaphore::signal() TA_REQ(!task::global_thread_lock)
{
	lock::lock_guard g{ task::global_thread_lock };
	signal_locked();
}

bool kbl::semaphore::try_wait_locked()
{
	global_thread_lock.assert_held();

	if (count_ > 0)
	{
		--count_;
		return true;
	}
	return false;
}


