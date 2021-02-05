#include "task/thread/thread.hpp"

#include "system/deadline.hpp"

using namespace task;

error_code wait_queue::unblock_thread(thread* t, error_code code) TA_REQ(global_thread_lock)
{

	return 0;
}

error_code wait_queue::block(interruptible intr) TA_REQ(global_thread_lock)
{
	return block_etc(0, resource_ownership::Normal, intr);
}

error_code wait_queue::block_etc(uint32_t signal_mask, resource_ownership reason, interruptible intr) TA_REQ(
	global_thread_lock)
{
	if (intr == interruptible::Yes && (cur_thread->signals_ & ~signal_mask))[[unlikely]]
	{
		if (cur_thread->signals_ & thread::SIGNAL_KILLED)
		{
			return ERROR_INTERNAL_INTR_KILLED;
		}
		else if (cur_thread->signals_ & thread::SIGNAL_SUSPEND)
		{
			return ERROR_INTERNAL_INTR_RETRY;
		}
	}

	cur_thread->wait_queue_state_->interruptible_ = intr;

	block_list_.push_back(cur_thread.get());

	if (reason == resource_ownership::Normal)
	{
		cur_thread->state = thread::thread_states::BLOCKED;
	}
	else if (reason == resource_ownership::Reader)
	{
		cur_thread->state = thread::thread_states::BLOCKED_READ_LOCK;
	}

	cur_thread->wait_queue_state_->blocking_on_ = this;
	cur_thread->wait_queue_state_->block_code_ = ERROR_SUCCESS;

	return ERROR_SUCCESS;
}

thread* wait_queue::peek() TA_REQ(global_thread_lock)
{
	return nullptr;
}

bool wait_queue::wake_one(bool reschedule, error_code code) TA_REQ(global_thread_lock)
{
	return false;
}

void wait_queue::wake_all(bool reschedule, error_code code) TA_REQ(global_thread_lock)
{

}

bool wait_queue::empty() const TA_REQ(global_thread_lock)
{
	return block_list_.empty();
}

bool wait_queue_state::holding() TA_REQ(global_thread_lock)
{
	return parent_->wait_queue_link.next != &parent_->wait_queue_link;
}

void wait_queue_state::block(wait_queue::interruptible intr, error_code status) TA_REQ(global_thread_lock)
{

}

void wait_queue_state::unblock_if_interruptible(thread* t, error_code status) TA_REQ(global_thread_lock)
{

}

bool wait_queue_state::unsleep(thread* thread, error_code status) TA_REQ(global_thread_lock)
{
	return false;
}

bool wait_queue_state::unsleep_if_interruptible(thread* thread, error_code status) TA_REQ(global_thread_lock)
{
	return false;
}