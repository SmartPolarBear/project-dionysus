//#include "server_syscalls.hpp"
//#include "debug_output.hpp"
#include "dionysus.hpp"

int main(int argc, char** argv)
{
	write_format("argc=%d, argv[0]=0x%x, argv[1]=0x%x\n", argc, argv[0], argv[1]);
	while (true);
	return 0;
}