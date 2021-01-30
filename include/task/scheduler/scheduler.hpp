#pragma once

#include "system/types.h"

#include "debug/thread_annotations.hpp"

#include "task/thread/thread.hpp"
#include "task/scheduler/round_rubin.hpp"

#include "ktl/string_view.hpp"
#include "ktl/list.hpp"

#include "kbl/data/list.hpp"

namespace task
{

struct scheduler_timer
{
	size_t expires{ 0 };
	thread* owner{ nullptr };

	kbl::list_link<scheduler_timer, lock::spinlock> link{ this };
};

class scheduler
{
 public:
	using scheduler_class_type = round_rubin_scheduler_class;
	using timer_list_type = kbl::intrusive_list<scheduler_timer, lock::spinlock, &scheduler_timer::link, true, false>;
	using size_type = size_t;

	friend class thread;

 public:

	scheduler() = delete;

	scheduler(const scheduler&) = delete;
	scheduler& operator=(const scheduler&) = delete;

	explicit scheduler(cpu_struct* cpu)
		: scheduler_class(this),
		  owner_cpu(cpu)
	{
	}

	[[noreturn]]  static error_code idle(void* arg __UNUSED);
	static_assert(ktl::Convertible<decltype(idle), task::thread::routine_type>);

	void schedule() TA_REQ(global_thread_lock);

	void reschedule() TA_EXCL(global_thread_lock);

	void yield() TA_EXCL(global_thread_lock);

	void unblock(thread* t) TA_EXCL(global_thread_lock);
	void insert(thread* t) TA_EXCL(global_thread_lock);

 public:

	struct current
	{
		static void schedule() TA_EXCL(global_thread_lock);

		static void reschedule() TA_EXCL(global_thread_lock);

		static void yield() TA_EXCL(global_thread_lock);

		static void unblock(thread* t) TA_EXCL(global_thread_lock);

		static void insert(thread* t) TA_EXCL(global_thread_lock);

		static void timer_tick_handle() TA_EXCL(global_thread_lock, timer_lock);
	};

 private:

	void enqueue(thread* t) TA_REQ(global_thread_lock);
	void dequeue(thread* t) TA_REQ(global_thread_lock);
	thread* fetch() TA_REQ(global_thread_lock);
	thread* steal() TA_REQ(global_thread_lock);
	void tick(thread* t)  TA_REQ(global_thread_lock, timer_lock);

	[[nodiscard]] size_type workload_size() const TA_REQ(global_thread_lock);

	void timer_tick_handle() TA_EXCL(global_thread_lock, timer_lock);

	timer_list_type timer_list TA_GUARDED(timer_lock) {};

	scheduler_class_type scheduler_class{ this };

	cpu_struct* owner_cpu{ (cpu_struct*)0xdeadbeef };

	mutable lock::spinlock timer_lock{ "scheduler timer" };
};

}