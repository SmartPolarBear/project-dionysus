#pragma once

#include "task/scheduler/scheduler_class.hpp"
#include "task/thread/thread.hpp"

#include "ktl/list.hpp"

namespace task
{
class ule_scheduler_class
	: public scheduler_class
{
 public:
	static constexpr uint64_t INTERACT_MAX = 100;
	static constexpr uint64_t INTERACT_HALF = INTERACT_MAX / 2;
	static constexpr uint64_t INTERACT_THRESHOLD = 30;

	ule_scheduler_class() = delete;
	~ule_scheduler_class() = default;
	ule_scheduler_class(const ule_scheduler_class&) = delete;
	ule_scheduler_class(ule_scheduler_class&&) = delete;
	ule_scheduler_class& operator=(const ule_scheduler_class&) = delete;

	explicit ule_scheduler_class(class scheduler* pa) : parent_(pa)
	{
	}

 public:
	[[nodiscard]] size_type workload_size() const override;
	void enqueue(thread* t) override;
	void dequeue(thread* t) override;
	thread* fetch() override;
	void tick() override;
	thread* steal(cpu_struct* stealer_cpu) override;

 private:
	class scheduler* parent_{ nullptr };
	mutable lock::spinlock lock_;
};

}