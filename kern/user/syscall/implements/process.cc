#include "syscall.h"
#include "syscall/syscall_args.hpp"

#include "system/mmu.h"
#include "system/syscall.h"

#include "debug/kdebug.h"
#include "drivers/apic/traps.h"

#include "arch/amd64/cpu/cpuid.h"
#include "arch/amd64/cpu/msr.h"
#include "arch/amd64/cpu/regs.h"

#include "task/process/process.hpp"

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
	auto heap_ptr = syscall::args_get<uintptr_t*, 0>(regs);
//	return cur_proc->resize_heap((uintptr_t*)get_nth_arg(regs, 0));
	return cur_proc->resize_heap(heap_ptr);
}

error_code sys_send(const syscall_regs* regs)
{
	// FIXME
	KDEBUG_NOT_IMPLEMENTED;
}

error_code sys_send_page(const syscall_regs* regs)
{    // FIXME

	KDEBUG_NOT_IMPLEMENTED;

}

error_code sys_receive(const syscall_regs* regs)
{    // FIXME

	KDEBUG_NOT_IMPLEMENTED;

}

error_code sys_receive_page(const syscall_regs* regs)
{    // FIXME

	KDEBUG_NOT_IMPLEMENTED;

}