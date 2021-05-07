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

	static constexpr int32_t PRIORITY_MIN = 0, PRIORITY_MAX = 255, THREAD_PRIO_MIN = -20, THREAD_PRIO_MAX = 20;

	static constexpr size_t SCHED_PRI_NRESV = THREAD_PRIO_MAX - PRIORITY_MIN;
	static constexpr size_t SCHED_PRI_NHALF = SCHED_PRI_NRESV / 2;

	enum priorities : int32_t
	{
		PRIORITY_MAX_IDLE = PRIORITY_MAX,
		PRIORITY_MIN_IDLE = 224,
		PRIORITY_IDLE_LEN = PRIORITY_MAX - PRIORITY_MIN_IDLE + 1,

		PRIORITY_MAX_TIMESHARE = PRIORITY_MIN_IDLE - 1,
		PRIORITY_MIN_TIMESHARE = 120,
		PRIORITY_TIMESHARE_LEN = PRIORITY_MAX_TIMESHARE - PRIORITY_MIN_TIMESHARE + 1,
		PRIORITY_INTERACT_LEN = (PRIORITY_TIMESHARE_LEN - SCHED_PRI_NRESV) / 2,
		PRIORITY_BATCH_LEN = (PRIORITY_TIMESHARE_LEN - PRIORITY_INTERACT_LEN),

		PRIORITY_MIN_INTERACT = PRIORITY_MIN_TIMESHARE,
		PRIORITY_MAX_INTERACT = PRIORITY_MIN_TIMESHARE + PRIORITY_INTERACT_LEN - 1,
		PRIORITY_MIN_BATCH = PRIORITY_MIN_TIMESHARE + PRIORITY_INTERACT_LEN,
		PRIORITY_MAX_BATCH = PRIORITY_MAX_TIMESHARE,

		PRIORITY_MAX_KERNEL = PRIORITY_MIN_TIMESHARE - 1,
		PRIORITY_MIN_KERNEL = 80,

		PRIORITY_MAX_REALTIME = PRIORITY_MIN_KERNEL - 1,
		PRIORITY_MIN_REALTIME = 48,
	};

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