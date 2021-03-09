#pragma once

#if defined(__ASSEMBLER__)

#define SYS_hello 1
#define SYS_exit 2

#elif defined(_DIONYSUS_KERNEL_)

#include "syscall/syscall.hpp"

#else

#include "syscall/public/syscall.hpp"

#endif