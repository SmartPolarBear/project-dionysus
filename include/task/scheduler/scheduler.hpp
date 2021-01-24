#pragma once

#include "system/types.h"

#include "debug/thread_annotations.hpp"

#include "task/thread/thread.hpp"
#include "task/scheduler/round_rubin.hpp"

#include "ktl/string_view.hpp"
#include "ktl/list.hpp"

#include "kbl/data/list_base.hpp"
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

	friend class thread;

 public:
	void schedule() TA_REQ(global_thread_lock);

	[[noreturn]]void reschedule() TA_EXCL(global_thread_lock);

	void yield() TA_EXCL(global_thread_lock);

	void unblock(thread* t) TA_REQ(global_thread_lock);
	void insert(thread* t) TA_REQ(global_thread_lock);

	void handle_timer_tick() TA_EXCL(global_thread_lock, timer_lock);

 private:

	scheduler_class_type scheduler_class{};

	void enqueue(thread* t) TA_REQ(global_thread_lock);
	void dequeue(thread* t) TA_REQ(global_thread_lock);
	thread* pick_next() TA_REQ(global_thread_lock);
	void timer_tick(thread* t) TA_REQ(global_thread_lock) TA_EXCL(timer_lock);

	timer_list_type timer_list TA_GUARDED(timer_lock) {};
	mutable lock::spinlock timer_lock{ "scheduler timer" };

	mutable lock::spinlock test{ "fuck!!" };
};

}