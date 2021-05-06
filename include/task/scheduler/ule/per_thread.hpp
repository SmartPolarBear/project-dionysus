#pragma once
#include "task/scheduler/base/per_thread.hpp"

namespace task
{

class ule_scheduler_state_base
	: public scheduler_state_base
{
 public:
	using interactivity_score_type = uint64_t;
	using nice_type = int32_t;

 public:
	bool nice_available() override
	{
		return true;
	}
	void on_tick() override;
	void on_sleep() override;
	void on_wakeup() override;

	int32_t get_nice() override
	{
		return nice_;
	}

	void set_nice(int32_t nice) override
	{
		nice_ = nice;
	}

	[[nodiscard]]interactivity_score_type interactivity_score() const;

 private:
	nice_type nice_{ 0 };
	interactivity_score_type interactivity_{ 0 };

	size_t run_time_{ 0 };

	size_t sleep_time_{ 0 };
	size_t sleep_tick_{ 0 };
};

}