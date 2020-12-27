#include "process.hpp"
#include "load_code.hpp"
#include "syscall.h"

#include "arch/amd64/cpu/cpu.h"
#include "arch/amd64/cpu/msr.h"

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

using libkernel::list_add;
using libkernel::list_empty;
using libkernel::list_for_each;
using libkernel::list_init;
using libkernel::list_remove;

using lock::spinlock;
using lock::spinlock_acquire;
using lock::spinlock_initialize_lock;
using lock::spinlock_release;
using lock::spinlock_holding;

using task::process_dispatcher;

using namespace memory;
using namespace vmm;

// send and receive message
error_code task::process_ipc_send(process_id pid, IN const void* message, size_t size)
{
	auto target = find_process(pid);
	if (target == nullptr)
	{
		return -ERROR_INVALID;
	}

	spinlock_acquire(&target->messaging_data.lock);

	while (target->messaging_data.data != nullptr)
		// wait until it is consumed
		;

	if (size >= target->messaging_data.INTERNAL_BUF_SIZE)
	{
		target->messaging_data.data = kmalloc(size, 0);
	}
	else
	{
		target->messaging_data.data = target->messaging_data.internal_buf;
	}

	memmove(target->messaging_data.data, message, size);

	target->messaging_data.data_size = size;

	process_wakeup((size_t)&target->messaging_data);

	spinlock_release(&target->messaging_data.lock);

	return ERROR_SUCCESS;
}

error_code task::process_ipc_receive(OUT void* message_out)
{
	spinlock_acquire(&cur_proc->messaging_data.lock);

	while (cur_proc->messaging_data.data == nullptr)
	{
		process_sleep((size_t)&cur_proc->messaging_data, &cur_proc->messaging_data.lock);
	}

	memmove(message_out, cur_proc->messaging_data.data, cur_proc->messaging_data.data_size);

	if (((uintptr_t)cur_proc->messaging_data.data) != (uintptr_t)(cur_proc->messaging_data.internal_buf))
	{
		kfree(cur_proc->messaging_data.data);
	}

	cur_proc->messaging_data.data = nullptr;
	cur_proc->messaging_data.data_size = 0;

	spinlock_release(&cur_proc->messaging_data.lock);

	return ERROR_SUCCESS;
}

error_code task::process_ipc_send_page(process_id pid, uint64_t unique_val, const void* page, size_t perm)
{
	auto target = find_process(pid);
	if (target == nullptr)
	{
		return -ERROR_INVALID;
	}

	spinlock_acquire(&target->messaging_data.lock);

	while (target->messaging_data.dst == nullptr)
		// wait until it consumed
		;

	target->messaging_data.unique_value = unique_val;
	target->messaging_data.perms = 0;

	// re-map the memory page
	if (page < (void*)USER_TOP && page != nullptr)
	{
		uintptr_t src_va = (uintptr_t)page;
		if (src_va % PAGE_SIZE)
		{
			spinlock_release(&target->messaging_data.lock);
			return -ERROR_INVALID;
		}

		constexpr size_t VALID_PERM_READONLY = PG_P | PG_U;
		constexpr size_t VALID_PERM_RW = PG_P | PG_U | PG_W;
		if (((perm & VALID_PERM_READONLY) != VALID_PERM_READONLY) || ((perm | VALID_PERM_RW) != VALID_PERM_RW))
		{
			spinlock_release(&target->messaging_data.lock);
			return -ERROR_INVALID;
		}

		auto pte = vmm::walk_pgdir(cur_proc->mm->pgdir, src_va, false);
		auto page = pmm::pde_to_page(pte);

		if (page == nullptr)
		{
			spinlock_release(&target->messaging_data.lock);
			return -ERROR_INVALID;
		}

		if ((perm & PG_W) && !(*pte & PG_W))
		{
			spinlock_release(&target->messaging_data.lock);
			return -ERROR_INVALID;
		}

		if (target->messaging_data.dst < (void*)USER_TOP)
		{
			auto ret = pmm::page_insert(target->mm->pgdir, true, page, (uintptr_t)target->messaging_data.dst, perm);
			if (ret != ERROR_SUCCESS)
			{
				spinlock_release(&target->messaging_data.lock);
				return ret;
			}
			target->messaging_data.perms = perm;
		}
	}

	target->messaging_data.can_receive = false;
	target->messaging_data.from = cur_proc->id;

	spinlock_release(&target->messaging_data.lock);

	process_wakeup((size_t)(&target->messaging_data.can_receive));

	return ERROR_SUCCESS;
}

error_code task::process_ipc_receive_page(void* out_page)
{
	if (out_page < (void*)USER_TOP && ((uintptr_t)out_page) % PAGE_SIZE)
	{
		return -ERROR_INVALID;
	}

	cur_proc->messaging_data.dst = out_page;

	spinlock_acquire(&cur_proc->messaging_data.lock);

	cur_proc->messaging_data.can_receive = true;

	while (cur_proc->messaging_data.can_receive)
	{
		process_sleep((size_t)(&cur_proc->messaging_data.can_receive), &cur_proc->messaging_data.lock);
	}

	spinlock_release(&cur_proc->messaging_data.lock);

	return ERROR_SUCCESS;
}


