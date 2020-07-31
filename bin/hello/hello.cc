#include "dionysus.hpp"

int main()
{
	for (;;)
	{
//		AddMessage add;
//		ipc_receive(&add);
//
//		AddRetMessage ret;
//		ret.ret = add.a + add.b;
//		ipc_send(0, &ret, sizeof(ret));

		AddMessage* add = new AddMessage{};
		ipc_receive(add);

		AddRetMessage* ret = new AddRetMessage{ .ret=add->a + add->b };
		ipc_send(0, ret, sizeof(AddRetMessage));

		delete add;
		delete ret;
	}
	return 0;
}