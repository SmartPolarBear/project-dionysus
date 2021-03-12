#include "syscall.h"

#include "system/mmu.h"
#include "system/syscall.h"

#include "debug/kdebug.h"
#include "drivers/apic/traps.h"

#include "arch/amd64/cpu/cpuid.h"
#include "arch/amd64/cpu/msr.h"
#include "arch/amd64/cpu/regs.h"

#include "builtin_text_io.hpp"

using namespace syscall;

#pragma clang diagnostic push

#pragma clang diagnostic ignored "-Wc99-designator"
#pragma clang diagnostic ignored "-Winitializer-overrides"

#include "syscall_handles.hpp"

extern "C" syscall_entry syscall_table[SYSCALL_COUNT_MAX + 1] = {
	// default for all
	[0] = invalid_syscall,
	[1 ... SYSCALL_COUNT_MAX] = default_syscall,

	// implemented
	[SYS_hello] = sys_hello,
	[SYS_exit] = sys_exit,
	[SYS_put_str] = sys_put_str,
	[SYS_put_char] = sys_put_char,
	[SYS_set_heap_size]=sys_set_heap,

	[SYS_ipc_load_message]= sys_ipc_load_message

};

#pragma clang diagnostic pop