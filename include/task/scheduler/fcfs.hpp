#pragma once

#include "task/scheduler/scheduler_class.hpp"
#include "task/thread/thread.hpp"

#include "ktl/list.hpp"

namespace task
{

class fcfs_scheduler_class final
	: public scheduler_class
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
	explicit fcfs_scheduler_class(class scheduler* pa) : parent_(pa)
	{
	}

	fcfs_scheduler_class() = delete;
	fcfs_scheduler_class(const fcfs_scheduler_class&) = delete;
	fcfs_scheduler_class(fcfs_scheduler_class&&) = delete;
	fcfs_scheduler_class& operator=(const fcfs_scheduler_class&) = delete;
	fcfs_scheduler_class& operator=(fcfs_scheduler_class&&) = delete;

	thread* steal(cpu_struct* stealer_cpu) final;

	void enqueue(thread* thread) final;

	size_type workload_size() const final;

	void dequeue(thread* thread) final;

	thread* fetch() final;

	void tick() final;

 private:
	class scheduler* parent_{ nullptr };

	run_queue_list_type run_queue_ TA_GUARDED(lock_);
	zombie_queue_list_type zombie_queue_ TA_GUARDED(lock_);

	mutable lock::spinlock lock_;
};

}