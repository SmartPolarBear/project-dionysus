#include "sys/types.h"

extern "C" int main(int argc, char **argv);

extern "C" void libmain(int argc, char **argv)
{
    main(argc, argv);
    //TODO: call exit() syscall
}