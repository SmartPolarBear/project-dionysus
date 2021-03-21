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

	lock::lock_guard g{ cur_thread->ipc_state_.lock_ };

	auto tag = msg->get_tag();
	cur_thread->ipc_state_.set_message_tag_locked(&tag);

	cur_thread->ipc_state_.load_mrs_locked(1, msg->get_items_span());

	return ERROR_SUCCESS;
}

error_code sys_ipc_send(const syscall_regs* regs)
{
	auto target_handle = args_get<object::handle_type, 0>(regs);

	auto handle_entry = cur_proc->handle_table()->get_handle_entry(target_handle);
	if (auto ret = cur_proc->handle_table()->object_from_handle<thread>(handle_entry);has_error(ret))
	{
		return get_error_code(ret);
	}
	else
	{
		auto th = get_result(ret);

		lock::lock_guard g1{ cur_thread->lock };
		lock::lock_guard g2{ th->lock };

		if (!global_thread_lock.holding())
		{
			lock::lock_guard g{ global_thread_lock };
			cur_thread->ipc_state_.send_locked(th, deadline::after(timeout));
		}
		else
		{
			global_thread_lock.assert_held();
			cur_thread->ipc_state_.send_locked(th, deadline::after(timeout));
		}

	}

	return ERROR_SUCCESS;
}

error_code sys_ipc_receive(const syscall_regs* regs)
{
	auto from_handle = args_get<object::handle_type, 0>(regs);
	auto timeout = args_get<time_type, 1>(regs);

	auto handle_entry = cur_proc->handle_table()->get_handle_entry(from_handle);
	if (auto ret = cur_proc->handle_table()->object_from_handle<thread>(handle_entry);has_error(ret))
	{
		return get_error_code(ret);
	}
	else
	{
		auto from = get_result(ret);

		lock::lock_guard g1{ cur_thread->lock };
		lock::lock_guard g2{ from->lock };

		if (!global_thread_lock.holding())
		{
			lock::lock_guard g{ global_thread_lock };
			cur_thread->ipc_state_.receive_locked(from, deadline::after(timeout));
		}
		else
		{
			global_thread_lock.assert_held();
			cur_thread->ipc_state_.send_locked(from, deadline::after(timeout));
		}

	}

	return ERROR_SUCCESS;
}