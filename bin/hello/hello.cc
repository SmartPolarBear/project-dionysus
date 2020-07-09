#include "io.hpp"

//TODO: syscall client can't handle parameter passing right.

extern "C" int main(int argc, const char** argv)
{
	write_format("%d %x %s\n", 1, 0x12345000, "fuck");
	return 0;
}