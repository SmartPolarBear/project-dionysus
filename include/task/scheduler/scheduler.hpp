#pragma once

#include "system/types.h"

#include "debug/thread_annotations.hpp"

#include "task/thread/thread.hpp"

namespace task
{

namespace scheduler2
{
class scheduler
{
 public:
	using size_type = size_t;
 public:
	scheduler() = default;
	~scheduler() = default;

	scheduler(const scheduler&) = delete;
	scheduler& operator=(const scheduler&) = delete;

	size_type get_runnable_tasks() const TA_EXCL(master_thread_lock);

	cpu_num_type get_cpu() const;

	// Public entry points.

	static void initialize_thread(thread* thread, int priority);
	static void block() TA_REQ(master_thread_lock);
	static void yield() TA_REQ(master_thread_lock);
	static void preempt() TA_REQ(master_thread_lock);
	static void reschedule() TA_REQ(master_thread_lock);
	static void reschedule_internal() TA_REQ(master_thread_lock);

	// Return true if the thread was placed on the current cpu's run queue.
	// This usually means the caller should locally reschedule soon.
	[[nodiscard]]static bool unblock(thread* thread)  TA_REQ(master_thread_lock);
	[[nodiscard]]static bool unblock(thread::list_type thread_list)  TA_REQ(master_thread_lock);
	static void unblock_idle(thread* idle_thread) TA_REQ(master_thread_lock);

 private:
	friend struct cpu_struct;
};

}

}