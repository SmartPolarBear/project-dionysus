#include "io.hpp"
#include "process.hpp"

int main()
{
	AddMessage add;
	ipc_receive(&add);

	AddRetMessage ret;
	ret.ret = add.a + add.b;
	ipc_send(0, &ret, sizeof(ret));
	return 0;
}