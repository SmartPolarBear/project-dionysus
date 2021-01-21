#pragma once

#include "system/types.h"

#include "debug/thread_annotations.hpp"

#include "task/thread/thread.hpp"
#include "task/scheduler/round_rubin.hpp"

#include "ktl/string_view.hpp"
#include "ktl/list.hpp"

#include "kbl/data/list_base.hpp"

namespace task
{

struct scheduler_timer
{
	size_t expires{ 0 };
	thread* owner{ nullptr };
	kbl::doubly_linked_node_state<scheduler_timer> ns;
};

using timer_list_type = kbl::intrusive_doubly_linked_list<scheduler_timer, &scheduler_timer::ns>;

class scheduler
{
 public:
	using scheduler_class_type = round_rubin_scheduler_class;

 public:
	void schedule() TA_REQ(global_thread_lock);

	[[noreturn]]void reschedule() TA_EXCL(global_thread_lock);

	void yield() TA_EXCL(global_thread_lock);

	void unblock(thread* t) TA_REQ(global_thread_lock);
	void insert(thread* t) TA_REQ(global_thread_lock);

	void handle_timer_tick() TA_EXCL(global_thread_lock, timer_lock);

 private:
	scheduler_class_type scheduler_class{};

	void enqueue(thread* t) TA_REQ(global_thread_lock);
	void dequeue(thread* t) TA_REQ(global_thread_lock);
	thread* pick_next() TA_REQ(global_thread_lock);
	void timer_tick(thread* t) TA_REQ(global_thread_lock, timer_lock);

 public:
	friend class thread;

	timer_list_type timer_list TA_GUARDED(timer_lock) {};
	mutable lock::spinlock timer_lock{ "scheduler timer" };

	// context context
	[[deprecated("no longer need. left for dead code. TO BE REMOVED")]]arch_task_context_registers* context{ nullptr };
};

}