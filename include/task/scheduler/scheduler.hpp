#pragma once

#include "system/types.h"

#include "debug/thread_annotations.hpp"

#include "task/thread/thread.hpp"

#include "ktl/string_view.hpp"
#include "ktl/list.hpp"

namespace task
{

class scheduler_class
{
 public:
	virtual ~scheduler_class() = default;

	virtual void enqueue(thread*) TA_REQ(global_thread_lock) = 0;

	virtual void dequeue(thread*) TA_REQ(global_thread_lock) = 0;

	virtual thread* pick_next() TA_REQ(global_thread_lock) = 0;

	virtual void timer_tick() TA_REQ(global_thread_lock) = 0;
};

class round_rubin_scheduler_class
	: public scheduler_class
{
 public:
	void enqueue(thread* thread) TA_REQ(global_thread_lock) override;

	void dequeue(thread* thread) TA_REQ(global_thread_lock) override;

	thread* pick_next() TA_REQ(global_thread_lock) override;

	void timer_tick() TA_REQ(global_thread_lock) override;

 private:
	ktl::list<thread*> run_queue TA_GUARDED(global_thread_lock);

};

class scheduler
{
 public:
	using class_type = round_rubin_scheduler_class;
 public:
	[[noreturn]] void schedule();

	void reschedule();
	void yield();

	void unblock(thread* t) TA_REQ(global_thread_lock);

 private:
	class_type scheduler_class{};

 public:
	friend class thread;

	// context context
	arch_task_context_registers* context{ nullptr };
};

}