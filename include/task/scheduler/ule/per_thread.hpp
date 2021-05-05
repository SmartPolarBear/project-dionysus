#pragma once
#include "task/scheduler/base/per_thread.hpp"

namespace task
{

class ule_scheduler_state_base
	: public scheduler_state_base
{
 public:
	bool nice_available() override
	{
		return true;
	}

	int32_t get_nice() override
	{
		return nice_;
	}

	void set_nice(int32_t nice) override
	{
		nice_ = nice;
	}
 private:
	int32_t nice_{ 0 };
};

}