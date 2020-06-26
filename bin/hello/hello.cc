#include "io.hpp"

//TODO: syscall client can't handle parameter passing right.

extern "C" int main(int argc, const char** argv)
{
	for (size_t i = 0; i < 0x7fffffff; i++)
	{
		hello(i + 1, i + 2, i + 3, i + 4);
	}

	return 0;
}