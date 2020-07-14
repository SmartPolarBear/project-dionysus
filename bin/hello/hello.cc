#include "io.hpp"

extern "C" int main(int argc, const char** argv)
{
//	write_format("%d %x %s\n", 1, 0x12345000, "fuck");
	for (size_t i = 0; i < 10000; i++)
	{
//		write_format("fuck %d times\n", i);
//		put_str("fuck ! fuck\n");
		char str[] = { ' ', 'f', 'u', 'c', 'k', '\n', '\0' };
		str[0] = (i % 10) + '0';
		put_str(str);
	}
	return 0;
}