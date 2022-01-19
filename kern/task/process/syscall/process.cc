#include "syscall.h"
#include "syscall/syscall_args.hpp"
#include "syscall/args_validation.hpp"

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

	auto handle = handle_table::get_global_handle_table()->query_handle([](const handle_entry& h)
	{
	  auto proc = downcast_dispatcher<process>(h.object());
	  return proc && proc->get_koid() == cur_proc->get_koid();
	});

	*out = cur_proc->handle_table_.add_handle(handle_entry::duplicate(handle));

	if (*out == INVALID_HANDLE_VALUE)
	{
		return -ERROR_INVALID;
	}

	return ERROR_SUCCESS;
}

error_code sys_get_process_by_id(const syscall_regs* regs)
{
	auto out = args_get<object::handle_type*, 0>(regs);
	auto pid = args_get<koid_type, 1>(regs);

	if (!arg_valid_pointer(out))
	{
		return -ERROR_INVALID;
	}

	auto pred = [id = pid](const handle_entry& h)
	{
	  auto proc = downcast_dispatcher<process>(h.object());
	  return proc && proc->get_koid() == id;
	};

	auto local_handle = cur_proc.is_valid() ?
	                    cur_proc->handle_table_.query_handle(pred) : nullptr;

	if (!local_handle)
	{
		auto handle = handle_table::get_global_handle_table()->query_handle(pred);

		if (!handle)
		{
			return -ERROR_NOT_EXIST;
		}

		*out = cur_proc->handle_table_.add_handle(handle_entry::duplicate(handle));
	}
	else
	{
		*out = cur_proc->handle_table_.entry_to_handle(local_handle);
	}

	if (*out == INVALID_HANDLE_VALUE)
	{
		return -ERROR_INVALID;
	}

	return ERROR_SUCCESS;
}

error_code sys_get_process_by_name(const syscall_regs* regs)
{
	auto out = args_get<object::handle_type*, 0>(regs);
	auto name = args_get<char*, 1>(regs);

	if (!arg_valid_pointer(out))
	{
		return -ERROR_INVALID;
	}

	if (!arg_valid_string(name))
	{
		return -ERROR_INVALID;
	}

	auto pred = [n = const_cast<const char*>(name)](const handle_entry& h)
	{
	  auto obj = downcast_dispatcher<process>(h.object());
	  return obj && obj->get_name().compare(n) == 0;
	};

	auto local_handle = cur_proc.is_valid() ?
	                    cur_proc->handle_table_.query_handle(pred) : nullptr;

	if (!local_handle)
	{
		auto handle = handle_table::get_global_handle_table()->query_handle(pred);

		if (!handle)
		{
			return -ERROR_NOT_EXIST;
		}

		*out = cur_proc->handle_table_.add_handle(handle_entry::duplicate(handle));
	}
	else
	{
		*out = cur_proc->handle_table_.entry_to_handle(local_handle);
	}

	if (*out == INVALID_HANDLE_VALUE)
	{
		return -ERROR_INVALID;
	}

	return ERROR_SUCCESS;
}


