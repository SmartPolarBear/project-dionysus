#include "dionysus.hpp"
#include <cstring>

uint8_t buf[4_MB] __attribute__((aligned(2_MB)));
#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
int main()
{
	while (true)put_str("iPCTEST\n");;
	while (true);
	while (true)
	{
		pid_type pid = 0;
		size_t perms = 0;
		uint64_t unique = 0;
		ipc_receive_page(buf, &unique, &pid, &perms);

		size_t value = 0;
		memmove(&value, buf, sizeof(value));

		write_format("value:%lld,page:%lld\n", unique, value);
	}
//	for (int i = 1; i < 1024; i++)
//	{
//		for (int j = 1; j < 1024; j++)
//		{
//
//			AddMessage add_msg;
//			add_msg.a = i;
//			add_msg.b = j;
//			ipc_send(1,&add_msg,sizeof(add_msg));
//
//			AddRetMessage ret_msg;
//			ipc_receive(&ret_msg);
//
//			if (ret_msg.ret == (i + j))
//			{
//				write_format("yes. %d+%d=%d\n", i, j, ret_msg.ret);
//			}
//			else
//			{
//				put_str("no\n");
//			}
//		}
//	}
	return 0;
}
#pragma clang diagnostic pop