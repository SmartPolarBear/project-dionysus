#pragma once

#include "system/types.h"

#include "debug/thread_annotations.hpp"

#include "task/thread/thread.hpp"

namespace task
{

namespace scheduler2
{
class scheduler
{

	friend struct cpu_struct;

	cpu_num_type this_cpu{ CPU_NUM_INVALID };
};

}

}