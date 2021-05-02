#include "task/scheduler/mlfq.hpp"

task::scheduler_class::size_type task::mlfq_scheduler::workload_size() const
{
	return 0;
}

void task::mlfq_scheduler::enqueue(task::thread* t)
{

}

void task::mlfq_scheduler::dequeue(task::thread* t)
{

}

task::thread* task::mlfq_scheduler::fetch()
{
	return nullptr;
}

void task::mlfq_scheduler::tick()
{

}

task::thread* task::mlfq_scheduler::steal(cpu_struct* stealer_cpu)
{
	return scheduler_class::steal(stealer_cpu);
}
