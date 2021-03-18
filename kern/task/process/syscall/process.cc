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

using namespace syscall;
using namespace object;

using namespace task;

error_code sys_exit(const syscall_regs* regs)
{
	auto code = static_cast<task_return_code>(syscall::args_get<0>(regs));
	cur_proc->exit(code); // this never return

	return -ERROR_SHOULD_NOT_REACH_HERE;
}

error_code sys_set_heap(const syscall_regs* regs)
{
	auto heap_ptr = syscall::args_get<uintptr_t*, 0>(regs);
	return cur_proc->resize_heap(heap_ptr);
}

error_code sys_get_current_process(const syscall_regs* regs)
{
	if (!cur_proc.is_valid())
	{
		return -ERROR_INVALID;
	}

	auto out = args_get<object::handle_type*, 0>(regs);

	if (!VALID_USER_PTR(out))
	{
		return -ERROR_INVALID;
	}

	auto handle = handle_entry::create("current_thread"sv, cur_proc.get());

	*out = cur_proc->handle_table_.add_handle(std::move(handle));

	return ERROR_SUCCESS;
}