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

void task::ipc_state::copy_mrs_to(thread* another, size_t st, size_t cnt)
{
	lock_guard g{ another->lock };

	register_t dummy = 0xdeadbeaf;
	asm volatile (
	"repnz movsq (%%rsi), (%%rdi)\n"
	: /* output */
	"=S"(dummy), "=D"(dummy), "=c"(dummy)
	: /* input */
	"c"(cnt), "S"(&mr_[st]),
	"D"(&another->ipc_state_.mr_[st]));
}

void task::ipc_state::set_message_tag(const ipc::message_tag* tag) noexcept
{
	mr_[0] = tag->raw();
	mr_count_ = 1;
}

void task::ipc_state::set_acceptor(const ipc::message_acceptor* acc) noexcept
{
	br_[0] = acc->raw();
	br_count_ = 1;
}