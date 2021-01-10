#include "process.hpp"

#include "drivers/apic/traps.h"

#include "task/scheduler/scheduler.hpp"
#include "task/thread/thread.hpp"
#include "task/thread/thread_dispatcher.hpp"
#include "task/process/process_dispatcher.hpp"

#include "system/process.h"

#include "kbl/lock/spinlock.h"

#include "ktl/mutex/lock_guard.hpp"
#include "ktl/algorithm.hpp"

using namespace lock;
using namespace task;
using namespace task::scheduler2;

using ktl::mutex::lock_guard;

spinlock task::master_thread_lock{ "thread master lock" };

ktl::list<task::thread*> task::thread_list TA_GUARDED(master_thread_lock);

cls_item<thread*, CLS_CUR_THREAD_PTR> task::thread::current::cur_thread;

// TODO: arch dependent
int idle_thread_start_routine(void* arg)
{
	hlt();
}

void thread_initialize(thread* t, thread_trampoline_routine_type trampoline)
{

}

// TODO: end arch dependent

void task::thread::default_trampoline()
{
	proc_list.lock.unlock();
	master_thread_lock.unlock();
}

task::thread::thread() = default;

thread::thread(ktl::string_view n) :
	name{ n }
{
}

task::thread::~thread() = default;

task::thread* task::thread::create_idle_thread(cpu_num_type cpuid)
{
	KDEBUG_ASSERT(cpuid != 0); // not boot cpu

	char name[16] = { 0 };
	sniprintf(name, sizeof(name), "idle %u", cpuid);

	auto th = thread::create_etc(&cpus[cpuid].idle, name, idle_thread_start_routine, nullptr, 0, nullptr);

	if (th == nullptr)
	{
		return th;
	}

	th->flags = static_cast<thread_flags>(THREAD_FLAG_IDLE | THREAD_FLAG_DETACHED);

	th->cpuid = cpuid;

	{
		lock_guard g{ master_thread_lock };
		scheduler2::scheduler::unblock_idle(th);
	}

	return th;
}

task::thread* task::thread::create_etc(task::thread* t,
	ktl::string_view name,
	task::thread_start_routine_type entry,
	void* arg,
	[[maybe_unused]]int priority,
	task::thread_trampoline_routine_type trampoline)
{
	size_t flags{ 0 };
	kbl::allocate_checker ck{};
	if (t)
	{
		t = new(&ck) thread{ name };
		if (!ck.check())
		{
			return nullptr;
		}

		flags |= THREAD_FLAG_FREE_STRUCT;
	}

	t->task_state_.init(entry, arg);
	scheduler2::scheduler::initialize_thread(t, priority);

	if (likely(trampoline == nullptr))
	{
		trampoline = &thread::default_trampoline;
	}

	thread_initialize(t, trampoline);

	{
		ktl::mutex::lock_guard g{ master_thread_lock };
		thread_list.push_back(t);
	}

	return nullptr;
}

task::thread* task::thread::create(ktl::string_view name,
	task::thread_start_routine_type entry,
	void* arg,
	int priority)
{
	return create_etc(nullptr, name, entry, arg, priority, nullptr);
}

void task::thread::resume()
{
	bool resched = false;

	// never reschedule into boot thread before idle thread set up
	if (!arch_ints_disabled())
	{
		resched = true;
	}

	{
		lock_guard g{ master_thread_lock };

		if (this->scheduler_state_.get_status() == scheduler_state::THREAD_DEATH)
		{
			return;
		}

		signals_.fetch_and(~THREAD_SIGNAL_SUSPEND, kbl::memory_order_relaxed);

		if (auto st = scheduler_state_.get_status();st == scheduler_state::THREAD_INITIAL
			|| st == scheduler_state::THREAD_SUSPENDED)
		{
			bool local_resched = scheduler2::scheduler::unblock(this);
			if (resched && local_resched)
			{
				scheduler2::scheduler::reschedule();
			}
		}
	}
}

error_code task::thread::suspend()
{
	if (is_idle())
	{
		return -ERROR_INVALID;
	}

	lock_guard g{ master_thread_lock };

	if (scheduler_state_.get_status() == scheduler_state::THREAD_DEATH)
	{
		return -ERROR_THREAD_STATE;
	}

	signals_.fetch_or(THREAD_SIGNAL_SUSPEND, kbl::memory_order_relaxed);

	bool local_resched = false;
	switch (scheduler_state_.get_status())
	{

	case scheduler_state::THREAD_INITIAL:
		local_resched = scheduler2::scheduler::unblock(this);
		break;
	case scheduler_state::THREAD_READY:
		// do nothing
		break;
	case scheduler_state::THREAD_RUNNING:
		// do nothing. it will happen sooner
		break;
	case scheduler_state::THREAD_BLOCKED:
	case scheduler_state::THREAD_BLOCKED_READ_LOCK:
		break;
	case scheduler_state::THREAD_SLEEPING:
		break;
	case scheduler_state::THREAD_SUSPENDED:
		// already suspended, do nothing
		break;
	case scheduler_state::THREAD_DEATH:
		KDEBUG_GERNERALPANIC_CODE(-ERROR_INVALID);
		break;
	}

	if (local_resched)
	{
		scheduler2::scheduler::reschedule();
	}

	return ERROR_SUCCESS;
}

void task::thread::forget()
{
	lock_guard g{ master_thread_lock };

	KDEBUG_ASSERT(thread::current::get() != this);

	this->erase_from_all_lists();

	KDEBUG_ASSERT(!wait_queue_state_.in_wait_queue());

	delete this;
}

error_code task::thread::detach()
{
	lock_guard g{ master_thread_lock };

	// other threads can't be blocked insider join() on this thread
	task_state_.wake_joiners(ERROR_THREAD_STATE);

	if (scheduler_state_.get_status() == scheduler_state::THREAD_DEATH)
	{
		flags &= ~THREAD_FLAG_DETACHED;
		g.unlock();
		return join(nullptr, 0);
	}
	else
	{
		flags |= THREAD_FLAG_DETACHED;
		return ERROR_SUCCESS;
	}

	return -ERROR_SHOULD_NOT_REACH_HERE;
}

error_code task::thread::detach_and_resume()
{
	if (auto err = detach();err != ERROR_SUCCESS)
	{
		return err;
	}
	resume();
	return ERROR_SUCCESS;
}

error_code task::thread::join(int* out_code, time_type deadline)
{
	{
		lock_guard g{ master_thread_lock };

		if (flags & THREAD_FLAG_DETACHED)
		{
			return -ERROR_THREAD_STATE;
		}

		if (scheduler_state_.get_status() != scheduler_state::THREAD_DEATH)
		{
			if (auto err = task_state_.join(deadline);err != ERROR_SUCCESS)
			{
				return err;
			}
		}

		KDEBUG_ASSERT(scheduler_state_.get_status() != scheduler_state::THREAD_DEATH);

		if (out_code)
		{
			*out_code = task_state_.get_ret_code();
		}

		erase_from_all_lists();
	}

	delete this;

	return 0;
}

void task::thread::kill()
{
	lock_guard g{ master_thread_lock };

	signals_.fetch_or(THREAD_SIGNAL_KILL, kbl::memory_order_relaxed);

	bool local_resched = false;

	if (this == current::get())
	{
		return;
	}

	switch (get_status())
	{

	case scheduler_state::THREAD_INITIAL:
		// do nothing
		break;
	case scheduler_state::THREAD_READY:
		// do nothing
		break;
	case scheduler_state::THREAD_RUNNING:
		// do nothing. will kill the next time entering the kernel
		break;
	case scheduler_state::THREAD_BLOCKED:
	case scheduler_state::THREAD_BLOCKED_READ_LOCK:
		if (auto err = wait_queue_state_.try_unblock(this, ERROR_INTERNAL_INTR_KILLED);err != ERROR_SUCCESS)
		{
			KDEBUG_GERNERALPANIC_CODE(err);
		}
		break;
	case scheduler_state::THREAD_SLEEPING:
		if (auto ret = wait_queue_state_.try_wakeup(this, ERROR_INTERNAL_INTR_KILLED);has_error(ret))
		{
			KDEBUG_GERNERALPANIC_CODE(get_error_code(ret));
		}
		else
		{
			local_resched = get_result(ret);
		}
		break;
	case scheduler_state::THREAD_SUSPENDED:
		local_resched = scheduler2::scheduler::unblock(this);
		break;
	case scheduler_state::THREAD_DEATH:
		// do nothing
		break;
	}

	if (local_resched)
	{
		scheduler2::scheduler::reschedule();
	}
}

void task::thread::erase_from_all_lists() TA_REQ(master_thread_lock)
{
	auto iter_this = ktl::find(thread_list.begin(), thread_list.end(), this);
	if (iter_this != thread_list.end())
	{
		thread_list.erase(iter_this);
	}
}

void task::thread::set_priority(int priority)
{
	lock_guard g{ master_thread_lock };
	scheduler2::scheduler::change_priority(this, priority);
}

ktl::string_view task::thread::get_owner_name()
{
	if (owner != nullptr)
	{
		return owner->get_parent()->get_name();
	}

	return "kernel";
}

bool thread::check_kill_signal() TA_REQ(master_thread_lock)
{
	KDEBUG_ASSERT(arch_ints_disabled());
	KDEBUG_ASSERT(master_thread_lock.holding());

	if (get_signals() & THREAD_SIGNAL_KILL)
	{
		KDEBUG_ASSERT(get_status() != scheduler_state::THREAD_DEATH);
		return true;
	}
	else
	{
		return false;
	}
}

task::thread* task::thread::current::get()
{
	return task::thread::current::cur_thread();
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

void task::task_state::init(task::thread_start_routine_type entry, void* arg)
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

wait_queue_state::~wait_queue_state()
{

}

void wait_queue_state::block(interruptible intr, error_code block_code) TA_REQ(master_thread_lock)
{

}

error_code wait_queue_state::try_unblock(thread* t, error_code block_code) TA_REQ(master_thread_lock)
{
	return 0;
}

bool wait_queue_state::wakeup(thread* t, error_code st) TA_REQ(master_thread_lock)
{
	return false;
}

error_code_with_result<bool> wait_queue_state::try_wakeup(thread* t, error_code st) TA_REQ(master_thread_lock)
{
	return error_code_with_result<bool>();
}

void wait_queue_state::update_priority_when_blocking(thread* t, int prio, propagating prop) TA_REQ(master_thread_lock)
{

}
