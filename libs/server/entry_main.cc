#include "server_syscalls.hpp"

#include "system/types.h"

extern "C" int main(int argc, char **argv);

extern "C" [[maybe_unused]] void libmain(int argc, char **argv)
{
    auto ret = static_cast<size_t>(main(argc, argv));
    //TODO: Notify kernel of the exiting of system server

	terminate(ret);
}