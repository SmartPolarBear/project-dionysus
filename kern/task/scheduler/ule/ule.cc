#include "task/scheduler/ule/ule.hpp"

task::scheduler_class::size_type task::ule_scheduler_class::workload_size() const
{
	return 0;
}

void task::ule_scheduler_class::enqueue(task::thread* t)
{

}

void task::ule_scheduler_class::dequeue(task::thread* t)
{

}

task::thread* task::ule_scheduler_class::fetch()
{
	return nullptr;
}

void task::ule_scheduler_class::tick()
{

}

task::thread* task::ule_scheduler_class::steal(cpu_struct* stealer_cpu)
{
	return scheduler_class::steal(stealer_cpu);
}
