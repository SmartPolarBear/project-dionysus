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

extern "C" syscall_entry syscall_table[SYSCALL_COUNT + 1] = {
	[0 ... SYSCALL_COUNT] = default_syscall,

	[SYS_hello] = sys_hello,
	[SYS_exit] = sys_exit,
	[SYS_put_str] = sys_put_str,
	[SYS_put_char] = sys_put_char,
	[SYS_send]=sys_send,
	[SYS_send_page]=sys_send_page,
	[SYS_receive]=sys_receive,
	[SYS_receive_page]=sys_receive_page,
	[SYS_set_heap_size]=sys_set_heap
};

#pragma clang diagnostic pop