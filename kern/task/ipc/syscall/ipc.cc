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
#include "task/thread/thread.hpp"
#include "task/ipc/message.hpp"

using namespace task;
using namespace syscall;

error_code sys_ipc_load_message(const syscall_regs* regs)
{
	auto msg = syscall::args_get<task::ipc::message*, 0>(regs);
	if (!VALID_USER_PTR(reinterpret_cast<uintptr_t>(msg)))
	{
		return -ERROR_INVALID;
	}

	global_thread_lock.assert_not_held();

	cur_thread->get_ipc_state()->load_message(msg);

	return ERROR_SUCCESS;
}

error_code sys_ipc_send(const syscall_regs* regs)
{
	auto target_handle = args_get<object::handle_type, 0>(regs);
	auto timeout = args_get<time_type, 1>(regs);

	auto proc = cur_proc.get();

	auto handle_entry = proc->handle_table()->get_handle_entry(target_handle);
	if (auto ret = proc->handle_table()->object_from_handle<thread>(handle_entry);has_error(ret))
	{
		return get_error_code(ret);
	}
	else
	{
		auto target = get_result(ret);

		global_thread_lock.assert_not_held();

		auto err = cur_thread->get_ipc_state()->send(target, deadline::after(timeout));
		if (err != ERROR_SUCCESS)
		{
			KDEBUG_GERNERALPANIC_CODE(err);
		}

		return err;
	}

	return ERROR_SUCCESS;
}

error_code sys_ipc_receive(const syscall_regs* regs)
{
	auto from_handle = args_get<object::handle_type, 0>(regs);
	auto timeout = args_get<time_type, 1>(regs);

	auto proc = cur_proc.get();

	auto handle_entry = proc->handle_table()->get_handle_entry(from_handle);
	if (auto ret = proc->handle_table()->object_from_handle<thread>(handle_entry);has_error(ret))
	{
		return get_error_code(ret);
	}
	else
	{
		auto from = get_result(ret);

		global_thread_lock.assert_not_held();

		auto err = cur_thread->get_ipc_state()->receive(from, deadline::after(timeout));
		if (err != ERROR_SUCCESS)
		{
			KDEBUG_GERNERALPANIC_CODE(err); //FIXME
		}

		return err;
	}

	return ERROR_SUCCESS;
}

error_code sys_ipc_store(const syscall_regs* regs)
{
	auto msg = syscall::args_get<task::ipc::message*, 0>(regs);

	if (!VALID_USER_PTR(reinterpret_cast<uintptr_t>(msg)))
	{
		return -ERROR_INVALID;
	}

	global_thread_lock.assert_not_held();

	cur_thread->get_ipc_state()->store_message(msg);

	return ERROR_SUCCESS;
}

error_code sys_ipc_accept(const syscall_regs* regs)
{
	auto acceptor = syscall::args_get<task::ipc::message_acceptor*, 0>(regs);

	if (!VALID_USER_PTR(reinterpret_cast<uintptr_t>(acceptor)))
	{
		return -ERROR_INVALID;
	}

	cur_thread->get_ipc_state()->set_acceptor(acceptor);

	return ERROR_SUCCESS;
}

error_code sys_ipc_wait(const syscall_regs* regs)
{
	auto timeout = args_get<time_type, 0>(regs);

	global_thread_lock.assert_not_held();

	auto err = cur_thread->get_ipc_state()->wait(deadline::after(timeout));
	if (err != ERROR_SUCCESS)
	{
		KDEBUG_GERNERALPANIC_CODE(err); //FIXME
	}

	return err;
}
