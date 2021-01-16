#pragma once

#include "system/types.h"

#include "debug/thread_annotations.hpp"

#include "task/thread/thread.hpp"

#include "ktl/string_view.hpp"

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
};


class scheduler
{
 public:
	[[noreturn]] static void schedule();

	static void reschedule();
	static void yield();

 private:
	round_rubin_scheduler_class scheduler_class{};
};


using default_scheduler_type = scheduler;

}