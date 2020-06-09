#include "io.hpp"

//TODO: syscall client can't handle parameter passing right.

extern "C" int main(int argc, const char **argv)
{
    size_t ret = hello(2001, 12, 04, 23);
    ret += hello(2002, 12, 04, 23);
    ret += hello(2003, 10, 01, 10);

    hello(ret, ret + ret, ret * ret, ret / 3);

    return 0;
}