#include "task/thread/thread.hpp"

using namespace task;

error_code wait_queue::unblock_thread(thread* t, error_code code) TA_REQ(global_thread_lock)
{
	return 0;
}

error_code wait_queue::block(const timer_t& deadline, wait_queue::Interruptible intr) TA_REQ(global_thread_lock)
{
	return 0;
}

error_code wait_queue::block_etc(const timer_t& deadline,
	uint32_t signal_mask,
	wait_queue::ResourceOwnership reason,
	wait_queue::Interruptible intr) TA_REQ(global_thread_lock)
{
	return 0;
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
	return false;
}
