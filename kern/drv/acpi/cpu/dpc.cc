#include "system/dpc.hpp"

error_code dpc::queue(bool resched)
{
	return 0;
}

error_code dpc::queue_thread_locked() TA_REQ(task::master_thread_lock)
{
	return 0;
}

void dpc::invoke()
{

}

void dpc_queue::initialize_for_current_cpu()
{

}
error_code dpc_queue::shutdown(time_type deadline)
{
	return 0;
}
void dpc_queue::transition_off_Cpu(dpc_queue& src)
{

}
void dpc_queue::enqueue(dpc* dpc)
{

}
void dpc_queue::signal(bool resched) TA_EXCL(task::master_thread_lock)
{

}
void dpc_queue::signal_locked() TA_REQ(task::master_thread_lock)
{

}
int dpc_queue::worker_thread(void*)
{
	return 0;
}
int dpc_queue::work()
{
	return 0;
}
