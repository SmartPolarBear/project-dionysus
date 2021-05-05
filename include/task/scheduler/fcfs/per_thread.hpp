#pragma once
#include "task/scheduler/base/per_thread.hpp"

namespace task
{

class fcfs_scheduler_state_base
	: public scheduler_state_base
{
 public:
	bool nice_available() override
	{
		return false;
	}

	int32_t get_nice() override
	{
		return 0;
	}

	void set_nice([[maybe_unused]]int32_t nice) override
	{
		KDEBUG_GERNERALPANIC_CODE(ERROR_UNSUPPORTED);
	}
};

}