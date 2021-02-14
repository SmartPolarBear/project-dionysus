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

	using run_queue_list_type = kbl::intrusive_list_with_default_trait<thread,
	                                                                   lock::spinlock,
	                                                                   &thread::run_queue_link,
	                                                                   true>;
	using zombie_queue_list_type = kbl::intrusive_list_with_default_trait<thread,
	                                                                      lock::spinlock,
	                                                                      &thread::zombie_queue_link,
	                                                                      true>;

 public:
	explicit round_rubin_scheduler_class(class scheduler* pa) : parent(pa)
	{
	}

	round_rubin_scheduler_class() = delete;
	round_rubin_scheduler_class(const round_rubin_scheduler_class&) = delete;
	round_rubin_scheduler_class(round_rubin_scheduler_class&&) = delete;
	round_rubin_scheduler_class& operator=(const round_rubin_scheduler_class&) = delete;
	round_rubin_scheduler_class& operator=(round_rubin_scheduler_class&&) = delete;

	thread* steal(cpu_struct* stealer_cpu) TA_REQ(global_thread_lock) override;

	void enqueue(thread* thread) TA_REQ(global_thread_lock) final;

	size_type workload_size() const TA_REQ(global_thread_lock) final;

	void dequeue(thread* thread) TA_REQ(global_thread_lock) final;

	thread* fetch() TA_REQ(global_thread_lock) final;

	void tick() TA_REQ(global_thread_lock) final;

 private:
	cpu_struct* owner_cpu{ nullptr };
	class scheduler* parent{ nullptr };

	run_queue_list_type run_queue TA_GUARDED(global_thread_lock);
	zombie_queue_list_type zombie_queue TA_GUARDED(global_thread_lock);
};

}