#include "io.hpp"

//TODO: syscall client can't handle parameter passing right.

extern "C" int main(int argc, const char** argv)
{

	size_t test = 100;
	test /= 0;

	size_t ret = hello(2001, 12, 04, 23);
	ret += hello(2002, 12, 04, 23);
	ret += hello(2003, 10, 01, 10);

	hello(ret, ret + ret, ret * ret, ret / 3);

	size_t i = 0;
	for (;;)
	{
		hello(i, i + i, i * i, i / 3);
		i++;
	}

	return 0;
}