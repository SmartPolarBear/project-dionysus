#include "syscall.h"

#include "system/mmu.h"
#include "system/process.h"
#include "system/syscall.h"

#include "drivers/debug/kdebug.h"
#include "drivers/apic/traps.h"

#include "arch/amd64/cpuid.h"
#include "arch/amd64/msr.h"
#include "arch/amd64/regs.h"

#include "libraries/libkernel/console/builtin_console.hpp"

using namespace syscall;

#pragma clang diagnostic push

#pragma clang diagnostic ignored "-Wc99-designator"
#pragma clang diagnostic ignored "-Winitializer-overrides"

#include "syscall_handles.hpp"

extern "C" syscall_entry syscall_table[SYSCALL_COUNT + 1] = {
        [0 ... SYSCALL_COUNT] = default_syscall,

        [SYS_hello] = sys_hello,
        [SYS_exit] = sys_exit,
        [SYS_put_str] = sys_put_str,
        [SYS_put_char] = sys_put_char,
};

#pragma clang diagnostic pop