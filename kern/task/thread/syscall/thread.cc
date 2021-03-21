#include "syscall.h"
#include "syscall/syscall_args.hpp"

#include "system/mmu.h"
#include "system/syscall.h"
#include "system/memlayout.h"

#include "debug/kdebug.h"
#include "drivers/apic/traps.h"

#include "arch/amd64/cpu/cpuid.h"
#include "arch/amd64/cpu/msr.h"
#include "arch/amd64/cpu/regs.h"

#include "task/process/process.hpp"
#include "task/thread/thread.hpp"
#include "task/ipc/message.hpp"

#include "object/handle_table.hpp"
#include "object/handle_entry.hpp"

using namespace task;
using namespace syscall;
using namespace object;

error_code sys_get_current_thread(const syscall_regs* regs)
{
	if (!cur_thread.is_valid() || !cur_proc.is_valid())
	{
		return -ERROR_INVALID;
	}

	auto out = args_get<object::handle_type*, 0>(regs);

	if(!VALID_USER_PTR(out))
	{
		return -ERROR_INVALID;
	}

	auto handle = handle_entry::create("current_thread"sv, cur_thread.get());

	*out = cur_proc->handle_table_.add_handle(std::move(handle));

	return ERROR_SUCCESS;
}

error_code sys_get_thread_by_id(const syscall_regs* regs)
{
	return ERROR_SUCCESS;
}

error_code sys_get_thread_by_name(const syscall_regs* regs)
{
	return ERROR_SUCCESS;
}