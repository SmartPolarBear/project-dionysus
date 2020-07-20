#include "io.hpp"
#include "process.hpp"

extern "C" int main(int argc, const char** argv)
{
	for (;;)
	{
		AddMessage add;
		initialize_message(add, kMsgTypeAddMessage, -1);
		ipc_receive(&add);

		AddRetMessage ret;
		ret.ret = add.a + add.b;
		initialize_message(ret, kMsgTypeAddRetMessage, 0);
		ipc_send(&ret);
	}
//	for (int i = 0; i < 0x7fffffff; i++)
//	{
//		put_str("be fucked\n");
//	}
	return 0;
}