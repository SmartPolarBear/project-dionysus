#pragma once

#include "task/scheduler/scheduler_class.hpp"
#include "task/thread/thread.hpp"

#include "ktl/list.hpp"

namespace task
{
class mlfq_scheduler
	:public scheduler_class
{
 public:
	[[nodiscard]] size_type workload_size() const override;
	void enqueue(thread* t) override;
	void dequeue(thread* t) override;
	thread* fetch() override;
	void tick() override;
	thread* steal(cpu_struct* stealer_cpu) override;
};
}