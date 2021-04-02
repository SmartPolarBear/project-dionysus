#include "dionysus.hpp"
#include <cstring>

using namespace task::ipc;
using namespace object;

handle_type get_receiver()
{
	handle_type thread = INVALID_HANDLE_VALUE;
	auto err = get_thread_by_name(&thread, "/hello");
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
	write_format("ERROR %d getting this process", err);
	while (true);
}

int main()
{
//	while(true)hello(9, 8, 7, 6);
	hello(9, 8, 7, 6);

	message msg{};

	message_tag tag{};

	tag.set_label(0x12345);

	msg.set_tag(tag);

	ipc_load_message(&msg);

	ipc_send(get_receiver(), TIME_INFINITE);

	return 0;
}
