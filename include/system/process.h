#pragma once

#include "arch/amd64/cpu/regs.h"

#include "system/mmu.h"
#include "system/types.h"
#include "system/vmm.h"
#include "system/syscall.h"
#include "system/messaging.hpp"
#include "system/cls.hpp"

#include "drivers/apic/traps.h"
#include "kbl/lock/spinlock.h"

#include "kbl/data/pod_list.h"

#include "task/process/process.hpp"
#include "task/job/job.hpp"

#include <cstring>
#include <algorithm>
#include <span>

namespace task
{
	void process_init();

	// FIXME
	using task::process;
	using task::binary_types;

	error_code process_load_binary(IN process* porc,
		IN uint8_t* bin,
		IN size_t binary_size,
		IN binary_types type,
		IN size_t flags);

	// handle task cleanup when exiting
	void process_exit(IN process* proc);

	// terminate current task
	error_code process_terminate(error_code error_code);

	// sleep on certain channel
	error_code process_sleep(size_t channel, lock::spinlock_struct* lock);

	// wake up child_processes sleeping on certain channel
	error_code process_wakeup(size_t channel);
	error_code process_wakeup_nolock(size_t channel);

	// send and receive message
	error_code process_ipc_send(pid_type pid, IN const void* message, size_t size);
	error_code process_ipc_receive(OUT void* message_out);
	// send and receive a page
	error_code process_ipc_send_page(pid_type pid, uint64_t unique_val, IN const void* page, size_t perm);
	error_code process_ipc_receive_page(OUT void* out_page);

	// allocate more memory
	error_code process_heap_change_size(IN process* proc, IN OUT uintptr_t* heap_ptr);

} // namespace task

//extern __thread task::process* current;
extern cls_item<task::process*, CLS_PROC_STRUCT_PTR> cur_proc;