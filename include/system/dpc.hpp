#pragma once

#include "ktl/list.hpp"

#include "task/thread/thread.hpp"

//class dpc final
//{
// public:
//	using func_type = void (*)(dpc*);
//
// public:
//
//	explicit dpc(func_type f, void* ar = nullptr)
//		: func_(f), arg_(ar)
//	{
//	}
//
//	template<typename T>
//	T* arg()
//	{
//		return static_cast<T*>(arg_);
//	}
//
//	error_code queue(bool resched);
//	error_code queue_thread_locked() TA_REQ(task::master_thread_lock);
//
// private:
//	friend class dpc_queue;
//	func_type func_;
//	void* arg_;
//
//	void invoke();
//};
//
//class dpc_queue final
//{
// public:
//	void initialize_for_current_cpu();
//
//	error_code shutdown(time_type deadline);
//
//	void transition_off_Cpu(dpc_queue& src);
//
//	void enqueue(dpc* dpc);
//	void signal(bool resched) TA_EXCL(task::master_thread_lock);
//
//	void signal_locked() TA_REQ(task::master_thread_lock);
//
// private:
//	static int worker_thread(void*);
//	int work();
//
//	cpu_num_type cpu{ CPU_NUM_INVALID };
//
//	bool initialized{ false };
//
//	bool stop{ false };
//
//	ktl::list<dpc*> list;
//
//	task::thread* thread;
//};
