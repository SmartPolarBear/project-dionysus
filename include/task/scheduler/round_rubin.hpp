#include "task/scheduler/scheduler_class_base.hpp"
#include "task/thread/thread.hpp"

#include "ktl/list.hpp"

namespace task
{
class round_rubin_scheduler_class final
	: public scheduler_class_base
{
 public:
	friend class thread;
	friend class scheduler;

	using run_queue_list_type = kbl::intrusive_list<thread, lock::spinlock, &thread::run_queue_link, true, false>;
	using zombie_queue_list_type = kbl::intrusive_list<thread, lock::spinlock, &thread::zombie_queue_link, true, false>;

	static size_t called_count;

	explicit round_rubin_scheduler_class(cpu_struct* cpu, class scheduler* pa) : owner_cpu(cpu), parent(pa)
	{
		magic = ++called_count;
	}

	round_rubin_scheduler_class() = delete;
	round_rubin_scheduler_class(const round_rubin_scheduler_class&) = delete;
	round_rubin_scheduler_class(round_rubin_scheduler_class&&) = delete;
	round_rubin_scheduler_class& operator=(const round_rubin_scheduler_class&) = delete;
	round_rubin_scheduler_class& operator=(round_rubin_scheduler_class&&) = delete;

 public:
	void enqueue(thread* thread) TA_REQ(global_thread_lock) final;

	void dequeue(thread* thread) TA_REQ(global_thread_lock) final;

	thread* pick_next() TA_REQ(global_thread_lock) final;

	void timer_tick() TA_REQ(global_thread_lock) final;

 private:
	cpu_struct* owner_cpu{ nullptr };
	class scheduler* parent{ nullptr };
	size_t magic{ 0 };

	run_queue_list_type run_queue TA_GUARDED(global_thread_lock);
	zombie_queue_list_type zombie_queue TA_GUARDED(global_thread_lock);
};

}