#pragma once

#include "task/scheduler/scheduler_class.hpp"
#include "task/thread/thread.hpp"

#include "ktl/list.hpp"

namespace task
{
class mlfq_scheduler_class
	: public scheduler_class
{
 public:
	static constexpr uint32_t PRIORITY_MIN = 0, PRIORITY_MAX = 7; // only 16 queues per core
 public:
	[[nodiscard]] size_type workload_size() const override;
	void enqueue(thread* t) override;
	void dequeue(thread* t) override;
	thread* fetch() override;
	void tick() override;
	thread* steal(cpu_struct* stealer_cpu) override;
};
}