#include "dionysus.hpp"

using namespace task::ipc;
using namespace object;

void test_this_proc()
{
	handle_type this_proc = INVALID_HANDLE_VALUE;
	auto err = get_current_process(&this_proc);
	if (err != ERROR_SUCCESS)
	{
		goto error;
	}

	if (this_proc == INVALID_HANDLE_VALUE)
	{
		goto error;
	}

	write_format("this handle : %lld!\n", this_proc);
	return;
error:
	write_format("ERROR %d getting this process", err);
	while (true);
}

void test_this_thread()
{
	handle_type this_thread = INVALID_HANDLE_VALUE;
	auto err = get_current_thread(&this_thread);
	if (err != ERROR_SUCCESS)
	{
		goto error;
	}

	if (this_thread == INVALID_HANDLE_VALUE)
	{
		goto error;
	}

	write_format("this thread handle : %lld!\n", this_thread);
	return;
error:
	write_format("ERROR %d getting this thread!", err);
	while (true);
}

void test_get_proc_by_name()
{
	handle_type process = INVALID_HANDLE_VALUE;
	auto err = get_process_by_name(&process, "ipctest");
	if (err != ERROR_SUCCESS)
	{
		goto error;
	}

	if (process == INVALID_HANDLE_VALUE)
	{
		goto error;
	}

	write_format("ipc_test process handle : %lld!\n", process);
	return;
error:
	write_format("ERROR %d getting ipc_test process!", err);
	while (true);
}

handle_type get_sender()
{
	handle_type thread = INVALID_HANDLE_VALUE;
	auto err = get_thread_by_name(&thread, "/ipctest");
	if (err != ERROR_SUCCESS)
	{
		goto error;
	}

	if (thread == INVALID_HANDLE_VALUE)
	{
		goto error;
	}

	write_format("sender handle : %lld \n", thread);
	return thread;
error:
	write_format("ERROR %d getting sender", err);
	while (true);
}

int test()
{
	hello(1, 2, 3, 4);

	message msg{};

	ipc_receive(get_sender(), TIME_INFINITE);

	ipc_store(&msg);

	msg.at<uint64_t>(0);

	int c = 0;
	while (msg.get_tag().label() != 0x12345)
	{
		ipc_store(&msg);
		c++;
	}

	hello(msg.get_tag().label(), msg.at<uint64_t>(0), 32, c);

	return c;
}

int main()
{
//	while (true)hello(1, 2, 3, 4);

//	test_this_proc();
//	test_this_thread();
//
//	test_get_proc_by_name();

//	for (int c = 0; c == 0; c = test())
//	{}
	test();

	return 0;

}
