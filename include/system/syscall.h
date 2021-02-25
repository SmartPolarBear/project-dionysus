#pragma once

#if defined(__ASSEMBLER__)

#define SYS_hello 1
#define SYS_exit 2

#else

#include "syscall/syscall.hpp"

#endif