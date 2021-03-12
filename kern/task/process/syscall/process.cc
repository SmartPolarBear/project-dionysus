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


error_code sys_exit(const syscall_regs* regs)
{
	auto code = static_cast<task::task_return_code>(syscall::args_get<0>(regs));
	cur_proc->exit(code); // this never return

	return -ERROR_SHOULD_NOT_REACH_HERE;
}

error_code sys_set_heap(const syscall_regs* regs)
{
	auto heap_ptr = syscall::args_get<uintptr_t*, 0>(regs);
	return cur_proc->resize_heap(heap_ptr);
}
