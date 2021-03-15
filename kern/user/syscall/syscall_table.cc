#include "syscall.h"
#include "syscall_handles.hpp"
#include "syscall/syscall_args.hpp"

#include "system/mmu.h"
#include "system/syscall.h"

#include "debug/kdebug.h"
#include "drivers/apic/traps.h"

#include "arch/amd64/cpu/cpuid.h"
#include "arch/amd64/cpu/msr.h"
#include "arch/amd64/cpu/regs.h"

#include "builtin_text_io.hpp"

using namespace syscall;

error_code zero_is_invalid_syscall(const syscall_regs*)
{
	KDEBUG_GENERALPANIC("syscall number 0 should never be called.");

	return -ERROR_SHOULD_NOT_REACH_HERE;
}

error_code default_syscall(const syscall_regs* regs)
{
	auto syscall_no = syscall::args_get<syscall::ARG_SYSCALL_NUM>(regs);//get_syscall_number(regs); // first parameter

	kdebug::kdebug_warning("The syscall %lld isn't yet defined.", syscall_no);

	return ERROR_SUCCESS;
}

#pragma clang diagnostic push

#pragma clang diagnostic ignored "-Wc99-designator"
#pragma clang diagnostic ignored "-Winitializer-overrides"

#include "syscall_handles.hpp"

extern "C" syscall_entry syscall_table[SYSCALL_COUNT_MAX + 1] = {
	// default for all
	[0] = zero_is_invalid_syscall,
	[1 ... SYSCALL_COUNT_MAX] = default_syscall,

	[SYS_hello] = sys_hello,
	[SYS_put_str] = sys_put_str,
	[SYS_put_char] = sys_put_char,

	[SYS_get_current_thread] = sys_get_current_thread,

	[SYS_get_current_process] = sys_get_current_process,
	[SYS_exit] = sys_exit,
	[SYS_set_heap_size]=sys_set_heap,

	[SYS_ipc_load_message] = sys_ipc_load_message,
	[SYS_ipc_send] = sys_ipc_send,
	[SYS_ipc_receive] = sys_ipc_receive
};

#pragma clang diagnostic pop
