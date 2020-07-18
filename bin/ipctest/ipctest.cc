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

void refreshbuf(int ret, char* txtbuf, size_t len)
{
	for (int i = 0; i < len; i++)txtbuf[i] = '\0';

	int t = 0;
	while (ret)
	{
		txtbuf[t++] = (ret % 10) + '0';
		ret /= 10;

		if (t + 1 >= len)break;
	}

	txtbuf[t] = '\n';
}

char txtbuf[32] = {};
extern "C" int main(int argc, const char** argv)
{

	for (int i = 1; i < 1024; i++)
	{
		for (int j = 1; j < 1024; j++)
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

			if (ret.r == (i + j))
			{
//				put_str("yes\n");
			}
			else
			{
//				put_str("no\n");
			}
		}
	}
	return 0;
}