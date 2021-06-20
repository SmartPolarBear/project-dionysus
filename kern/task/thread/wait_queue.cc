
#include <task/thread/wait_queue.hpp>

#include "task/thread/thread.hpp"

#include "drivers/acpi/cpu.h"

#include "system/deadline.hpp"

#include "drivers/cmos/rtc.hpp"

#include "ktl/move.hpp"

using namespace task;

kbl::list_link<thread, lock::spinlock>& wait_queue_list_node_trait::node_link(thread& t)
{
	return t.wait_queue_link;
}

kbl::list_link<thread, lock::spinlock>& wait_queue_list_node_trait::node_link(thread* t)
{
	return t->wait_queue_link;
}

error_code wait_queue::unblock_thread(thread* t, error_code code) TA_REQ(global_thread_lock)
{
	KDEBUG_ASSERT(arch_ints_disabled());
	KDEBUG_ASSERT(global_thread_lock.holding());

	if (t->state != thread::thread_states::BLOCKED && t->state == thread::thread_states::BLOCKED_READ_LOCK)
	{
		return ERROR_INVALID;
	}

	auto wq = t->wait_queue_state_.blocking_on_;
	KDEBUG_ASSERT(wq != nullptr);
	KDEBUG_ASSERT(t->wait_queue_state_.holding());

	wq->dequeue(t, code);

	if (scheduler::current::unblock(t))
	{
		scheduler::current::reschedule_locked();
	}

	return ERROR_SUCCESS;
}

error_code wait_queue::block(interruptible intr) TA_REQ(global_thread_lock)
{
	return block_etc(deadline::infinite(), 0, resource_ownership::Normal, intr);
}

error_code wait_queue::block(wait_queue::interruptible intr, const deadline& ddl) TA_REQ(global_thread_lock)
{
	return block_etc(ddl, 0, resource_ownership::Normal, intr);
}

error_code wait_queue::block_etc(const deadline& ddl,
	uint32_t signal_mask,
	resource_ownership reason,
	interruptible intr) TA_REQ(
	global_thread_lock)
{
	auto current_thread = cur_thread.get();
	current_thread->scheduler_state_.on_sleep();

	KDEBUG_ASSERT(arch_ints_disabled());
	KDEBUG_ASSERT(current_thread->state == thread::thread_states::RUNNING);

	if (ddl.when() != TIME_INFINITE && ddl.when() < (time_type)cmos::cmos_read_rtc_timestamp())
	{
		return ERROR_TIMEOUT;
	}

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

	cur_thread->wait_queue_state_.interruptible_ = intr;

	block_list_.push_back(cur_thread.get());

	if (reason == resource_ownership::Normal)
	{
		cur_thread->state = thread::thread_states::BLOCKED;
	}
	else if (reason == resource_ownership::Reader)
	{
		cur_thread->state = thread::thread_states::BLOCKED_READ_LOCK;
	}

	cur_thread->wait_queue_state_.blocking_on_ = this;
	cur_thread->wait_queue_state_.block_code_ = ERROR_SUCCESS;

	if (ddl.when() != TIME_INFINITE)
	{
		scheduler_timer timer;

		timer.arg = current_thread;
		timer.callback = timeout_handle;
		timer.expires = ddl.when() - cmos::cmos_read_rtc_timestamp();

		cpu->scheduler->add_timer(&timer);
	}

	scheduler::current::block_locked();

	current_thread->wait_queue_state_.interruptible_ = interruptible::No;

	return current_thread->wait_queue_state_.block_code_;
}

thread* wait_queue::peek() TA_REQ(global_thread_lock)
{
	if (block_list_.empty())
	{
		return nullptr;
	}

	auto t = block_list_.front_ptr();
	return t;
}

bool wait_queue::wake_one(bool reschedule, error_code code) TA_REQ(global_thread_lock)
{
	bool woke = false;
	KDEBUG_ASSERT(arch_ints_disabled());
	KDEBUG_ASSERT(global_thread_lock.holding());

	auto t = peek();
	if (t)
	{
		dequeue(t, code);

		t->scheduler_state_.on_wakeup();

		bool local_reschedule = scheduler::current::unblock(t);

		if (reschedule && local_reschedule)
		{
			scheduler::current::reschedule_locked();
			woke = true;
		}
	}

	return woke;
}

void wait_queue::wake_all(bool reschedule, error_code code) TA_REQ(global_thread_lock)
{
	KDEBUG_ASSERT(arch_ints_disabled());
	KDEBUG_ASSERT(global_thread_lock.holding());

	if (block_list_.empty())
	{
		return;
	}

	decltype(block_list_) list;
	thread* t = nullptr;
	while ((t = peek()) != nullptr)
	{
		dequeue(t, code);
		list.push_back(t);
	}

	bool local_resched = scheduler::current::unblock_locked(ktl::move(list));
	if (reschedule && local_resched)
	{
		scheduler::current::reschedule_locked();
	}
}

bool wait_queue::empty() const TA_REQ(global_thread_lock)
{
	return block_list_.empty();
}

void wait_queue::dequeue(thread* t, error_code err) TA_REQ(global_thread_lock)
{
	KDEBUG_ASSERT(t != nullptr);
	KDEBUG_ASSERT(t->wait_queue_state_.holding());
	KDEBUG_ASSERT(t->state == thread::thread_states::BLOCKED || t->state == thread::thread_states::BLOCKED_READ_LOCK);
	KDEBUG_ASSERT(t->wait_queue_state_.blocking_on_ == this);

	block_list_.remove(t);
	t->wait_queue_state_.block_code_ = err;
	t->wait_queue_state_.blocking_on_ = nullptr;
}

void wait_queue::timeout_handle(scheduler_timer* timer, [[maybe_unused]] time_type time, void* arg)
{
	auto t = reinterpret_cast<thread*>(arg);

	cpu->scheduler->remove_timer(timer);

	unblock_thread(t, ERROR_TIMEOUT);

	global_thread_lock.unlock();
}

wait_queue::~wait_queue()
{
	if (!block_list_.empty())
	{
		KDEBUG_GENERALPANIC("wait_locked queue is destructed before it become empty");
	}
}

wait_queue_state::~wait_queue_state()
{
	KDEBUG_ASSERT(blocking_on_ == nullptr);
}

bool wait_queue_state::holding() TA_REQ(global_thread_lock)
{
	return !parent_->wait_queue_link.is_empty_or_detached();
}

void wait_queue_state::block(wait_queue::interruptible intr, error_code status) TA_REQ(global_thread_lock)
{
	block_code_ = status;
	interruptible_ = intr;
	scheduler::current::block_locked();
	interruptible_ = wait_queue::interruptible::No;
}

void wait_queue_state::unblock_if_interruptible(thread* t, error_code status) TA_REQ(global_thread_lock)
{
	if (interruptible_ == wait_queue::interruptible::Yes)
	{
		wait_queue::unblock_thread(t, status);
	}
}

bool wait_queue_state::unsleep(thread* thread, error_code status) TA_REQ(global_thread_lock)
{
	block_code_ = status;
	return scheduler::current::unblock(thread);
}

bool wait_queue_state::unsleep_if_interruptible(thread* thread, error_code status) TA_REQ(global_thread_lock)
{
	if (interruptible_ == wait_queue::interruptible::Yes)
	{
		return unsleep(thread, status);
	}
	return false;
}
