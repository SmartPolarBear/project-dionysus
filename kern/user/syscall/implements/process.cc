#include "syscall.h"

#include "system/mmu.h"
#include "system/process.h"
#include "system/syscall.h"

#include "debug/kdebug.h"
#include "drivers/apic/traps.h"

#include "arch/amd64/cpu/cpuid.h"
#include "arch/amd64/cpu/msr.h"
#include "arch/amd64/cpu/regs.h"

#include "builtin_text_io.hpp"

// take the following argument:
//  1. exit code, the value of which is error_codes
error_code sys_exit(const syscall_regs* regs)
{
	//FIXME
	KDEBUG_NOT_IMPLEMENTED;
}

error_code sys_set_heap(const syscall_regs* regs)
{
	return task::process_heap_change_size(cur_proc(), (uintptr_t*)get_nth_arg(regs, 0));
}

error_code sys_send(const syscall_regs* regs)
{
	return task::process_ipc_send((pid_type)get_nth_arg(regs, 0),
		(void*)get_nth_arg(regs, 1),
		(size_t)get_nth_arg(regs, 2));
}

error_code sys_send_page(const syscall_regs* regs)
{
	return task::process_ipc_send_page((pid_type)get_nth_arg(regs, 0),
		(uint64_t)get_nth_arg(regs, 1),
		(void*)get_nth_arg(regs, 2),
		(size_t)get_nth_arg(regs, 3));
}

error_code sys_receive(const syscall_regs* regs)
{
	return task::process_ipc_receive((void*)get_nth_arg(regs, 0));
}

error_code sys_receive_page(const syscall_regs* regs)
{
	auto ret = task::process_ipc_receive_page((void*)get_nth_arg(regs, 0));

	// FIXME

//	uint64_t* out_val = (uint64_t*)get_nth_arg(regs, 1);
//	*out_val = cur_proc->messaging_data.unique_value;
//
//	pid_type* out_pid = (pid_type*)get_nth_arg(regs, 2);
//	*out_pid = cur_proc->messaging_data.from;
//
//	size_t* out_perms = (size_t*)get_nth_arg(regs, 3);
//	*out_perms = cur_proc->messaging_data.perms;

	return ret;
}