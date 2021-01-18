#include "task/scheduler/scheduler_class_base.hpp"

namespace task
{
class round_rubin_scheduler_class final
	: public scheduler_class_base
{
 public:
	friend class thread;

 public:
	void enqueue(thread* thread) TA_REQ(global_thread_lock) override final;

	void dequeue(thread* thread) TA_REQ(global_thread_lock) override final;

	thread* pick_next() TA_REQ(global_thread_lock) override final;

	void timer_tick() TA_REQ(global_thread_lock) override final;

 private:
	ktl::list<thread*> run_queue TA_GUARDED(global_thread_lock);

};

}