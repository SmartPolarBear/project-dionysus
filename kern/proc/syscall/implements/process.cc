#include "syscall.h"

#include "system/mmu.h"
#include "system/process.h"
#include "system/syscall.h"

#include "drivers/debug/kdebug.h"
#include "drivers/apic/traps.h"

#include "arch/amd64/cpuid.h"
#include "arch/amd64/msr.h"
#include "arch/amd64/regs.h"

#include "libkernel/console/builtin_text_io.hpp"

// take the following argument:
//  1. exit code, the value of which is error_codes
error_code sys_exit(const syscall_regs* regs)
{
	return process::process_terminate((error_code)get_nth_arg(regs, 0));
}

error_code sys_set_heap(const syscall_regs* regs)
{
	return process::process_heap_change_size(cur_proc(), (uintptr_t*)get_nth_arg(regs, 0));
}

error_code sys_send(const syscall_regs* regs)
{
	return process::process_ipc_send((process_id)get_nth_arg(regs, 0),
		(void*)get_nth_arg(regs, 1),
		(size_t)get_nth_arg(regs, 2));
}

error_code sys_send_page(const syscall_regs* regs)
{
	return process::process_ipc_send_page((process_id)get_nth_arg(regs, 0),
		(uint64_t)get_nth_arg(regs, 1),
		(void*)get_nth_arg(regs, 2),
		(size_t)get_nth_arg(regs, 3));
}

error_code sys_receive(const syscall_regs* regs)
{
	return process::process_ipc_receive((void*)get_nth_arg(regs, 0));
}

error_code sys_receive_page(const syscall_regs* regs)
{
	auto ret = process::process_ipc_receive_page((void*)get_nth_arg(regs, 0));

	uint64_t* out_val = (uint64_t*)get_nth_arg(regs, 1);
	*out_val = cur_proc->messaging_data.unique_value;

	process_id* out_pid = (process_id*)get_nth_arg(regs, 2);
	*out_pid = cur_proc->messaging_data.from;

	size_t* out_perms = (size_t*)get_nth_arg(regs, 3);
	*out_perms = cur_proc->messaging_data.perms;

	return ret;
}