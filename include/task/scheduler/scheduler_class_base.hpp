#pragma once

namespace task
{
class thread;

class scheduler_class_base
{
 public:
	friend class thread;
 public:
	virtual ~scheduler_class_base() = default;

	virtual void enqueue(thread*) TA_REQ(global_thread_lock) = 0;

	virtual void dequeue(thread*) TA_REQ(global_thread_lock) = 0;

	virtual thread* pick_next() TA_REQ(global_thread_lock) = 0;

	virtual void timer_tick() TA_REQ(global_thread_lock) = 0;
};

}