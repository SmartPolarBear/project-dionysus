#include "syscall.h"
#include "syscall/syscall_args.hpp"
#include "syscall/args_validation.hpp"

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
#include "object/object_manager.hpp"

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

	if (!VALID_USER_PTR(out))
	{
		return -ERROR_INVALID;
	}

	auto id = cur_thread->get_koid();
	auto pred = [id](const handle_entry& h)
	{
	  auto t = downcast_dispatcher<task::thread>(h.object());
	  return t && t->get_koid() == id;
	};


	auto local_handle = cur_proc.is_valid() ?
	                    cur_proc->handle_table_.query_handle(pred) : nullptr;

	if (!local_handle)
	{
		auto handle = object_manager::global_handles()->query_handle(pred);
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

error_code sys_get_thread_by_id(const syscall_regs* regs)
{
	auto out = args_get<object::handle_type*, 0>(regs);
	auto id = args_get<koid_type, 1>(regs);

	if (!arg_valid_pointer(out))
	{
		return -ERROR_INVALID;
	}

	auto pred = [id](const handle_entry& h)
	{
	  auto t = downcast_dispatcher<task::thread>(h.object());
	  return t && t->get_koid() == id;
	};


	auto local_handle = cur_proc.is_valid() ?
	                    cur_proc->handle_table_.query_handle(pred) : nullptr;

	if (!local_handle)
	{
		auto handle = object_manager::global_handles()->query_handle(pred);
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

error_code sys_get_thread_by_name(const syscall_regs* regs)
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
	  auto obj = downcast_dispatcher<thread>(h.object());
	  return obj ? obj->get_name().compare(n) == 0 : false;
	};

	auto local_handle = cur_proc.is_valid() ?
	                    cur_proc->handle_table_.query_handle(pred) : nullptr;

	if (!local_handle)
	{
		auto handle = object_manager::global_handles()->query_handle(pred);
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
