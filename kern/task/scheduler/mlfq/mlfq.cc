#include "task/scheduler/mlfq.hpp"

task::scheduler_class::size_type task::mlfq_scheduler_class::workload_size() const
{
	return 0;
}

void task::mlfq_scheduler_class::enqueue(task::thread* t)
{

}

void task::mlfq_scheduler_class::dequeue(task::thread* t)
{

}

task::thread* task::mlfq_scheduler_class::fetch()
{
	return nullptr;
}

void task::mlfq_scheduler_class::tick()
{

}

task::thread* task::mlfq_scheduler_class::steal(cpu_struct* stealer_cpu)
{
	return scheduler_class::steal(stealer_cpu);
}
