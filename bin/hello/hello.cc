#include "io.hpp"
#include "process.hpp"

enum MessageType : int
{
	ADDMSG = 1,
	RETMSG
};

struct message_header
{
	int type;
}__attribute__((packed));

struct add_message
{
	message_header hdr;
	int a;
	int b;
}__attribute__((packed));

struct ret_message
{
	message_header hdr;
	int r;
}__attribute__((packed));


extern "C" int main(int argc, const char** argv)
{
	for (;;)
	{
		add_message add;
		size_t sz = 0;
		auto addp = &add;
		auto szp = &sz;

		receive((void**)&addp, szp);

		ret_message ret;
		ret.r = add.a + add.b;
		ret.hdr.type = RETMSG;

		send(1, sizeof(ret), &ret);
	}
	return 0;
}