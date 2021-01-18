#pragma once

#include "system/types.h"

#include "debug/thread_annotations.hpp"

#include "task/thread/thread.hpp"
#include "task/scheduler/round_rubin.hpp"

#include "ktl/string_view.hpp"
#include "ktl/list.hpp"

namespace task
{

class scheduler
{
 public:
	using class_type = round_rubin_scheduler_class;
 public:
	void schedule() TA_EXCL(global_thread_lock);
	[[noreturn]]void reschedule() TA_EXCL(global_thread_lock);

	void yield() TA_EXCL(global_thread_lock);

	void unblock(thread* t) TA_REQ(global_thread_lock);
	void insert(thread* t) TA_REQ(global_thread_lock);

	void handle_timer();

 private:
	class_type scheduler_class{};

	void enqueue(thread* t) TA_REQ(global_thread_lock);
	void dequeue(thread* t) TA_REQ(global_thread_lock);
	thread* pick_next() TA_REQ(global_thread_lock);
	void timer_tick(thread* t) TA_REQ(global_thread_lock);

 public:
	friend class thread;

	// context context
	arch_task_context_registers* context{ nullptr };
};

}