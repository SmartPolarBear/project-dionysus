#include "dionysus.hpp"
#include "system/types.h"
extern "C" void* memmove(void* s1, const void* s2, size_t n);


uint8_t buf[4_MB] __attribute__((aligned(2_MB)));

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
int main()
{
	while (true)put_str("hello");
	size_t num = 1234567;
	for (;;)
	{
		AddMessage add;
		ipc_receive(&add);

		AddRetMessage ret;
		ret.ret = add.a + add.b;
		ipc_send(0, &ret, sizeof(ret));

//		AddMessage* add = new AddMessage{};
//		ipc_receive(add);
//
//		AddRetMessage* ret = new AddRetMessage{ .ret=add->a + add->b };
//		ipc_send(0, ret, sizeof(AddRetMessage));
//
//		delete add;
//		delete ret;

//		memmove(buf, &num, sizeof(num));
//		ipc_send_page(0, 12345, buf, 7);
//		num++;
	}
	return 0;
}
#pragma clang diagnostic pop