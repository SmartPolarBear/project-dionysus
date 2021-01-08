#include "task/scheduler/scheduler.hpp"

using namespace task;
using namespace task::scheduler2;

[[nodiscard]] task::scheduler2::scheduler::size_type task::scheduler2::scheduler::get_runnable_tasks() const
TA_EXCL(master_thread_lock)
{
	return 0;
}

void task::scheduler2::scheduler::initialize_thread(task::thread* thread, int priority)
{

}

void task::scheduler2::scheduler::block() TA_REQ(master_thread_lock)
{

}

void task::scheduler2::scheduler::yield() TA_REQ(master_thread_lock)
{

}

void task::scheduler2::scheduler::preempt() TA_REQ(master_thread_lock)
{

}

void task::scheduler2::scheduler::reschedule() TA_REQ(master_thread_lock)
{

}

void task::scheduler2::scheduler::reschedule_internal() TA_REQ(master_thread_lock)
{

}

bool task::scheduler2::scheduler::unblock(task::thread* thread) TA_REQ(master_thread_lock)
{
	return false;
}

bool task::scheduler2::scheduler::unblock(task::thread::list_type thread_list) TA_REQ(master_thread_lock)
{
	return false;
}

void task::scheduler2::scheduler::unblock_idle(task::thread* idle_thread) TA_REQ(master_thread_lock)
{

}
