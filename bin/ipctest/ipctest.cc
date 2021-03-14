#include "dionysus.hpp"
#include <cstring>

using namespace task::ipc;

int main()
{
	hello(9, 8, 7, 6);

	message msg{};
	message_tag tag{};

	tag.set_label(0x12345);

	msg.set_tag(tag);

	ipc_load_message(&msg);

	return 0;
}
