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
	for (int i = 0; i < 1024; i++)
	{
		for (int j = 0; j < 1024; j++)
		{
			add_message msg;
			msg.hdr.type = ADDMSG;
			msg.a = i;
			msg.b = j;

			send(0, sizeof(msg), &msg);

			ret_message ret;
			size_t sz = 0;
			auto retp = &ret;
			auto szp = &sz;

			receive((void**)&retp, szp);

			write_format("ret number %d\n", ret.r);
		}
	}
	return 0;
}