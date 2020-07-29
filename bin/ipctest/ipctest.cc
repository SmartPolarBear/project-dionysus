#include "dionysus.hpp"

int main()
{

	for (int i = 1; i < 1024; i++)
	{
		for (int j = 1; j < 1024; j++)
		{

			AddMessage add_msg;
			add_msg.a = i;
			add_msg.b = j;
			ipc_send(1,&add_msg,sizeof(add_msg));

			AddRetMessage ret_msg;
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
	return 0;
}