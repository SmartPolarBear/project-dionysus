#include "task/thread.hpp"
#include "task/thread_dispatcher.hpp"

#include "kbl/lock/spinlock.h"

using namespace lock;

spinlock task::master_thread_lock;

task::thread* task::thread::create_idle_thread(cpu_num_type cpuid)
{
	return nullptr;
}

task::thread::thread()
{

}

task::thread::~thread()
{

}

task::thread* task::thread::create_etc(task::thread* t,
	const char* name,
	task::thread_start_routine entry,
	void* agr,
	int priority,
	task::thread_trampoline_routine trampoline)
{
	return nullptr;
}

task::thread* task::thread::create(ktl::string_view name, task::thread_start_routine entry, void* arg, int priority)
{
	return nullptr;
}

void task::thread::resume()
{

}

error_code task::thread::suspend()
{
	return 0;
}

void task::thread::forget()
{

}

error_code task::thread::detach()
{
	return 0;
}

error_code task::thread::detach_and_resume()
{
	return 0;
}

error_code task::thread::join(int* retcode, time_t deadline)
{
	return 0;
}

void task::thread::kill()
{

}

void task::thread::erase_from_all_lists() TA_REQ(master_thread_lock)
{

}

void task::thread::set_priority(int priority)
{

}

ktl::string_view task::thread::get_owner_name()
{
	return std::string_view();
}

void task::task_state::init(task::thread_start_routine entry, void* arg)
{

}

error_code task::task_state::join(time_t deadline) TA_REQ(master_thread_lock)
{
	return 0;
}

error_code task::task_state::wake_joiners(error_code status) TA_REQ(master_thread_lock)
{
	return 0;
}
task::thread* task::thread::current::get()
{
	return nullptr;
}
void task::thread::current::yield()
{

}
void task::thread::current::preempt()
{

}
void task::thread::current::reschedule()
{

}
void task::thread::current::exit(int retcode)
{

}
void task::thread::current::exit_locked(int retcode) TA_REQ(lock)
{

}
void task::thread::current::becomde_idle()
{

}
void task::thread::current::do_suspend()
{

}
void task::thread::current::set_name(const char* name)
{

}
