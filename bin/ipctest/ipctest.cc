#include "dionysus.hpp"
#include <cstring>

using namespace task::ipc;
using namespace object;

handle_type get_hello()
{
	handle_type thread = INVALID_HANDLE_VALUE;
	auto err = get_thread_by_name(&thread, "hello");
	if (err != ERROR_SUCCESS)
	{
		goto error;
	}

	if (thread == INVALID_HANDLE_VALUE)
	{
		goto error;
	}

	write_format("receiver handle : %lld \n", thread);
	return thread;
error:
	write_format("ERROR %d getting hello", err);
	while (true);
}

int main()
{
//	while(true)hello(9, 8, 7, 6);

//	while (true)
//	{
//		hello(9, 8, 7, 6);
//
//		message msg{};
//
//		message_tag tag{};
//		uint64_t untyped1 = 12345ull;
//
//		tag.set_label(0x12345);
//
//		msg.set_tag(tag);
//
//		msg.append(untyped1);
//
//		ipc_load_message(&msg);
//
//		ipc_send(get_receiver(), TIME_INFINITE);
//
//		hello(92, 82, 72, 62);
//	}

	auto hello_handle = get_hello();

	handle_type this_handle = INVALID_HANDLE_VALUE;
	if (get_current_process(&this_handle) != ERROR_SUCCESS)
	{
		put_str("Error getting handle of ipctest!");
	}

	for (int i = 2; i <= 10; i++)
	{
		message msg{};
		message_tag tag{};
		tag.set_label(i);

		msg.append(this_handle);
		msg.append(i);

		ipc_load_message(&msg);

		ipc_send(hello_handle, TIME_INFINITE);

		ipc_receive(hello_handle, TIME_INFINITE);

		message reply{};
		ipc_store(&reply);

		hello(i, reply.at<uint64_t>(0), 0, 0);
	}

	return 0;
}
