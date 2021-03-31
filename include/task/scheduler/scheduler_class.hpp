#pragma once
#include "task/thread/thread.hpp"

namespace task
{
class thread;

class scheduler_class
{
 public:
	friend class thread;

	using size_type = size_t;
 public:
	scheduler_class() = default;

	scheduler_class(const scheduler_class&) = delete;
	scheduler_class(scheduler_class&&) = delete;
	scheduler_class& operator=(const scheduler_class&) = delete;

	virtual ~scheduler_class() = default;

	[[nodiscard]] virtual size_type workload_size() const = 0;

	/// \brief add a thread to the run queue
	/// \param t
	virtual void enqueue(thread* t) = 0;

	/// \brief remove the thread from run queue
	/// \param t
	virtual void dequeue(thread* t) = 0;

	/// \brief get a thread, generally for running
	/// \return
	virtual thread* fetch() = 0;

	/// \brief handle the timer tick
	virtual void tick() = 0;

	/// \brief get a thread, generally for migrating to another CPU's scheduler
	/// \return
	virtual thread* steal(cpu_struct* stealer_cpu);

};

}