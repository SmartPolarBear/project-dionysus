#include "io.hpp"
#include "process.hpp"

extern "C" int main(int argc, const char** argv)
{
	for (;;)
	{
		AddMessage add;
		ipc_receive(&add);

		AddRetMessage ret;
		ret.ret = add.a + add.b;
		ipc_send(0, &ret, sizeof(ret));
	}
	return 0;
}