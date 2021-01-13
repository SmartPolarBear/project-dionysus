#pragma once

#include "system/types.h"

#include "debug/thread_annotations.hpp"

#include "task/thread/thread.hpp"

namespace task
{

class scheduler
{
 public:
	static void schedule();

	static void reschedule();
	static void yield();
};

}