#include "process.hpp"
#include "load_code.hpp"
#include "syscall.h"

#include "arch/amd64/cpu.h"
#include "arch/amd64/msr.h"

#include "system/error.hpp"
#include "system/kmalloc.hpp"
#include "system/memlayout.h"
#include "system/pmm.h"
#include "system/process.h"
#include "system/vmm.h"
#include "system/scheduler.h"

#include "drivers/apic/traps.h"
#include "drivers/lock/spinlock.h"

#include "kbl/pod_list.h"
#include "../../libs/basic_io/include/builtin_text_io.hpp"

#include "ktl/mutex/lock_guard.hpp"

using namespace libkernel;
using namespace lock;
using namespace memory;
using namespace vmm;
using namespace ktl::mutex;
using namespace task;

using namespace task;


error_code process_dispatcher::setup_kernel_stack()
{
	auto raw_stack = this->kstack.get();

	auto sp = reinterpret_cast<uintptr_t>(raw_stack + task::process_dispatcher::KERNSTACK_SIZE);

	sp -= sizeof(*this->tf);
	this->tf = reinterpret_cast<decltype(this->tf)>(sp);

	sp -= sizeof(uintptr_t);
	*((uintptr_t*)sp) = reinterpret_cast<uintptr_t>(raw_stack);

	sp -= sizeof(uintptr_t);
	*((uintptr_t*)sp) = (uintptr_t)user_proc_entry;

	sp -= sizeof(*this->context);
	this->context = reinterpret_cast<decltype(this->context)>(sp);
	memset(this->context, 0, sizeof(*this->context));

	this->context->rip = (uintptr_t)new_proc_begin;

	return ERROR_SUCCESS;
}

error_code process_dispatcher::setup_registers()
{
	tf->cs = SEGMENT_VAL(SEGMENTSEL_UCODE, DPL_USER);
	tf->ss = SEGMENT_VAL(SEGMENTSEL_UDATA, DPL_USER);
	tf->rsp = USER_STACK_TOP;
	tf->rflags |= trap::EFLAG_IF;

	if ((flags & PROC_SYS_SERVER) || (flags & PROC_DRIVER))
	{
		tf->rflags |= trap::EFLAG_IOPL_MASK;
	}
}

task::process_dispatcher::process_dispatcher(std::span<char> name, process_id id, process_id parent_id, size_t flags)
	: task_dispatcher(name, id, parent_id, flags)
{

}

error_code process_dispatcher::initialize()
{
	lock_guard guard{ proc_list.lockable };

	this->kstack = std::make_unique<uint8_t>(KERNSTACK_SIZE);
	if (auto ret = this->setup_kernel_stack();ret != ERROR_SUCCESS)
	{
		return ret;
	}

	if (auto ret = setup_mm();ret != ERROR_SUCCESS)
	{
		return ret;
	}

	if (auto ret = setup_registers();ret != ERROR_SUCCESS)
	{
		return ret;
	}

	return 0;
}

error_code process_dispatcher::setup_mm()
{
	return 0;
}
error_code process_dispatcher::load_binary(uint8_t* bin, size_t binary_size, binary_types type, size_t flags)
{
	return 0;
}
error_code process_dispatcher::terminate(error_code terminate_error)
{
	return 0;
}
error_code process_dispatcher::exit()
{
	return 0;
}
error_code process_dispatcher::sleep(sleep_channel_type channel, lock::spinlock* lk)
{
	return 0;
}
error_code process_dispatcher::wakeup(sleep_channel_type channel)
{
	return 0;
}
error_code process_dispatcher::wakeup_no_lock(sleep_channel_type channel)
{
	return 0;
}
error_code process_dispatcher::change_heap_ptr(uintptr_t* heap_ptr)
{
	return 0;
}
