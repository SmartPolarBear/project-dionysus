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

using scheduler_timer_callback = void (*)(struct scheduler_timer* timer, time_type time, void* arg);

struct scheduler_timer
{
	int64_t expires{ 0 };
	void* arg;
	scheduler_timer_callback callback;

	kbl::list_link<scheduler_timer, lock::spinlock> link{ this };
};

class scheduler
{
 public:
	using scheduler_class_type = round_rubin_scheduler_class;
	using timer_list_type = kbl::intrusive_list_with_default_trait<scheduler_timer,
	                                                               lock::spinlock,
	                                                               &scheduler_timer::link,
	                                                               true>;
	using size_type = size_t;

	friend class thread;

	friend scheduler_class_type;

 public:

	scheduler() = delete;

	scheduler(const scheduler&) = delete;
	scheduler& operator=(const scheduler&) = delete;

	explicit scheduler(cpu_struct* cpu)
		: owner_cpu(cpu), scheduler_class(this)
	{
	}

	[[noreturn]]  static error_code idle(void* arg __UNUSED);
	static_assert(ktl::convertible_to<decltype(idle), task::thread_routine_type>);

	void schedule() TA_REQ(global_thread_lock);

	void reschedule() TA_REQ(!global_thread_lock);
	void reschedule_locked() TA_REQ(global_thread_lock);

	void yield() TA_REQ(!global_thread_lock);
	void yield_locked() TA_REQ(global_thread_lock);

	void unblock_locked(thread* t) TA_REQ(global_thread_lock);

	void insert(thread* t) TA_REQ(!global_thread_lock);
	void insert_locked(thread* t) TA_REQ(global_thread_lock);

	void add_timer(scheduler_timer* timer);

	void remove_timer(scheduler_timer* timer);

 public:

	struct current
	{
		static void reschedule() TA_REQ(!global_thread_lock);

		static void reschedule_locked()TA_REQ(global_thread_lock);

		static void yield() TA_REQ(!global_thread_lock);

		static bool unblock(thread* t) TA_REQ(global_thread_lock);

		static bool unblock_locked(wait_queue::wait_queue_list_type threads) TA_REQ(global_thread_lock);

		static void insert(thread* t) TA_REQ(!global_thread_lock);

		static void block_locked() TA_REQ(global_thread_lock);

		static void timer_tick_handle() TA_REQ(!global_thread_lock, !timer_lock);

		[[noreturn]]static void enter() TA_EXCL(global_thread_lock);
	};

 private:
	static constexpr uintptr_t INVALID_PTR_MAGIC = 0xdeadbeef;

	void enqueue(thread* t);
	void dequeue(thread* t);
	thread* fetch();
	thread* steal();
	void tick(thread* t);

	void check_timers_locked() TA_REQ(timer_lock);

	[[nodiscard]] size_type workload_size() const TA_REQ(!global_thread_lock);
	[[nodiscard]] size_type workload_size_locked() const TA_REQ(global_thread_lock);

	void timer_tick_handle() TA_REQ(!global_thread_lock, !timer_lock);

	cpu_struct* owner_cpu{ (cpu_struct*)INVALID_PTR_MAGIC };

	scheduler_class_type scheduler_class{ this };

	timer_list_type timer_list TA_GUARDED(timer_lock) {};

	mutable lock::spinlock timer_lock{ "scheduler_timer" };
};

}