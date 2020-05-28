#include "syscall.hpp"

extern "C" int main(int argc, const char **argv)
{
    hello(2001, 12, 04, 23);
    hello(2002, 12, 04, 23);
    hello(2003, 10, 01, 10);

    return 0;
}