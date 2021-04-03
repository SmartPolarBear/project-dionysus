#include "../include/syscall.h"
#include "internals/thread.hpp"

#include "task/thread/thread.hpp"
#include "task/process/process.hpp"
#include "task/scheduler/scheduler.hpp"

#include "system/mmu.h"
#include "system/vmm.h"
#include "system/kmalloc.hpp"
#include "system/scheduler.h"
#include "system/deadline.hpp"

#include "drivers/acpi/cpu.h"

#include "kbl/lock/lock_guard.hpp"

#include <gsl/util>

#include <utility>

using namespace task;

using lock::lock_guard;

void task::ipc_state::copy_mrs_to_locked(thread* another, size_t st, size_t cnt)
{
	register_t dummy = 0xdeadbeaf;
	asm volatile (
	"repnz movsq (%%rsi), (%%rdi)\n"
	: /* output */
	"=S"(dummy), "=D"(dummy), "=c"(dummy)
	: /* input */
	"c"(cnt), "S"(&mr_[st]),
	"D"(&another->ipc_state_.mr_[st]));
}

void task::ipc_state::load_mrs_locked(size_t start, ktl::span<ipc::message_register_type> mrs) TA_REQ(parent_->lock)
{
	register_t dummy = 0xdeadbeaf;
	asm volatile (
	"repnz movsq (%%rsi), (%%rdi)\n"
	: /* output */
	"=S"(dummy), "=D"(dummy), "=c"(dummy)
	: /* input */
	"c"(mrs.size()), "S"(mrs.data()),
	"D"(&mr_[start]));
}

void task::ipc_state::set_message_tag_locked(const ipc::message_tag* tag) noexcept TA_REQ(parent_->lock)
{
	mr_[0] = tag->raw();
	mr_count_ = 1;
}

void task::ipc_state::set_acceptor(const ipc::message_acceptor* acc) noexcept
{
	br_[0] = acc->raw();
	br_count_ = 1;
}

error_code task::ipc_state::receive_locked(thread* from, const deadline& ddl)
{
	this->state_ = IPC_RECEIVING;

	error_code err = ERROR_SUCCESS;
	while (from->ipc_state_.state_ != IPC_SENDING)
	{
		err = from->ipc_state_.receive_wait_queue_.block(wait_queue::interruptible::Yes, ddl);
	}

	lock_guard g{ parent_->lock };

	send_wait_queue_.wake_all(true, ERROR_SUCCESS);
	return err;
}

error_code task::ipc_state::send_locked(thread* to, const deadline& ddl)
{
	this->state_ = IPC_SENDING;

	error_code err = ERROR_SUCCESS;
	while (to->ipc_state_.state_ != IPC_RECEIVING)
	{
		err = to->ipc_state_.send_wait_queue_.block(wait_queue::interruptible::Yes, ddl);
	}

	lock_guard g{ parent_->lock };

	copy_mrs_to_locked(to, 0, task::ipc_state::MR_SIZE);

	receive_wait_queue_.wake_all(true, ERROR_SUCCESS);
	return err;

}
