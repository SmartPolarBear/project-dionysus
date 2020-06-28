#include "io.hpp"

//TODO: syscall client can't handle parameter passing right.

extern "C" int main(int argc, const char** argv)
{
	put_str("hello world!");
	return 0;
}