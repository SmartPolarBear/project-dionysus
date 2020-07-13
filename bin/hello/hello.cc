#include "io.hpp"

extern "C" int main(int argc, const char** argv)
{
//	write_format("%d %x %s\n", 1, 0x12345000, "fuck");
	for (size_t i = 0; i < 10000; i++)
	{
//		write_format("fuck %d times\n", i);
		put_str("fuck ! fuck\n");
	}
	return 0;
}