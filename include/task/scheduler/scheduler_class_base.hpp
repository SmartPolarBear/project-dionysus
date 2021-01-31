#pragma once
#include "task/thread/thread.hpp"

namespace task
{
class thread;

class scheduler_class_base
{
 public:
	friend class thread;

	using size_type = size_t;
 public:
	virtual ~scheduler_class_base() = default;

	[[nodiscard]] virtual size_type workload_size() const = 0;

	/// \brief add a thread to the run queue
	/// \param t
	virtual void enqueue(thread* t) TA_REQ(global_thread_lock) = 0;

	/// \brief remove the thread from run queue
	/// \param t
	virtual void dequeue(thread* t) TA_REQ(global_thread_lock) = 0;

	/// \brief get a thread, generally for running
	/// \return
	virtual thread* fetch() TA_REQ(global_thread_lock) = 0;

	/// \brief get a thread, generally for migrating to another CPU's scheduler
	/// \return
	virtual thread* steal(cpu_struct* stealer_cpu) TA_REQ(global_thread_lock) = 0;

	/// \brief handle the timer tick
	virtual void tick() TA_REQ(global_thread_lock) = 0;
};

}