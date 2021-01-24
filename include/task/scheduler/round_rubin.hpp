#include "task/scheduler/scheduler_class_base.hpp"
#include "task/thread/thread.hpp"

namespace task
{
class round_rubin_scheduler_class final
	: public scheduler_class_base
{
 public:
	friend class thread;
	using run_queue_list_type = kbl::intrusive_list<thread, lock::spinlock, &thread::run_queue_link, true, false>;

 public:
	void enqueue(thread* thread) TA_REQ(global_thread_lock) final;

	void dequeue(thread* thread) TA_REQ(global_thread_lock) final;

	thread* pick_next() TA_REQ(global_thread_lock) final;

	void timer_tick() TA_REQ(global_thread_lock) final;

 private:

	run_queue_list_type run_queue TA_GUARDED(global_thread_lock);

};

}