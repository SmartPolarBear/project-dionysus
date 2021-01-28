#pragma once
#include "task/thread/thread.hpp"

namespace task
{
class thread;

class scheduler_class_base
{
 public:
	friend class thread;
 public:
	virtual ~scheduler_class_base() = default;

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
	virtual thread* steal() TA_REQ(global_thread_lock) = 0;

	/// \brief handle the timer tick
	virtual void tick() TA_REQ(global_thread_lock) = 0;
};

}