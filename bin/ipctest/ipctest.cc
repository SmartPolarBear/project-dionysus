#include "io.hpp"
#include "process.hpp"

extern "C" int main(int argc, const char** argv)
{

	for (int i = 1; i < 1024; i++)
	{
		for (int j = 1; j < 1024; j++)
		{

			AddMessage add_msg;
			initialize_message(add_msg, kMsgTypeAddMessage, 1);
			add_msg.a = i;
			add_msg.b = j;
			ipc_send(&add_msg);

			AddRetMessage ret_msg;
			initialize_message(ret_msg, kMsgTypeAddRetMessage, -1);
			ipc_receive(&ret_msg);

			if (ret_msg.ret == (i + j))
			{
				write_format("yes. %d+%d=%d\n", i, j, ret_msg.ret);
			}
			else
			{
				put_str("no\n");
			}
		}
	}
//
//	for (int i = 0; i < 0x7fffffff; i++)
//	{
//		put_str("fuck \n");
//	}
	return 0;
}