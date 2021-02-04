#include "task/thread/thread.hpp"

using namespace task;

error_code wait_queue::unblock_thread(thread* t, error_code code) TA_REQ(global_thread_lock)
{
	return 0;
}

error_code wait_queue::block(interruptible intr) TA_REQ(global_thread_lock)
{
	return 0;
}

error_code wait_queue::block_etc(uint32_t signal_mask, resource_ownership reason, interruptible intr) TA_REQ(global_thread_lock)
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
	return list.empty();
}
