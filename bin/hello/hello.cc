#include "dionysus.hpp"

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

int main()
{
	hello(1, 2, 3, 4);

	test_this_proc();
	test_this_thread();

	return 0;

}
