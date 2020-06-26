#pragma once

#include "arch/amd64/regs.h"

#include "system/mmu.h"
#include "system/types.h"
#include "system/vmm.h"
#include "system/syscall.h"

#include "drivers/apic/traps.h"

#include <cstring>
#include <algorithm>

namespace process
{
	enum process_state
	{
		PROC_STATE_UNUSED,
		PROC_STATE_EMBRYO,
		PROC_STATE_SLEEPING,
		PROC_STATE_RUNNABLE,
		PROC_STATE_RUNNING,
		PROC_STATE_ZOMBIE,
	};

	enum process_flags
	{
		PROC_EXITING = 0b1,
		PROC_SYS_SERVER = 0x10,
		PROC_USER = 0b100,
		PROC_DRIVER = 0b1000,
	};

	constexpr size_t PROC_WAITING_INTERRUPTED = 0x80000000;
	enum process_waiting_state : size_t
	{
		PROC_WAITING_NONE,
		PROC_WAITING_CHILD = 0x1ul | PROC_WAITING_INTERRUPTED,
		PROC_WAITING_TIMER = 0x2ul | PROC_WAITING_INTERRUPTED,
	};

	using pid = int64_t;

	constexpr size_t PROC_NAME_LEN = 32;

// it should be enough
	constexpr size_t PROC_MAX_COUNT = INT32_MAX;

	constexpr pid PID_MAX = INT64_MAX;

// Per-process state
	struct process_dispatcher
	{
		static constexpr size_t KERNSTACK_PAGES = 2;
		static constexpr size_t KERNSTACK_SIZE = KERNSTACK_PAGES * PAGE_SIZE;

		char name[PROC_NAME_LEN + 1];

		process_state state;

		pid id;
		pid parent_id;

		size_t runs;
		uintptr_t kstack;

		vmm::mm_struct* mm;

		trap::trap_frame trapframe;
		uintptr_t pgdir_addr;

		size_t flags;
		size_t wating_state;
		error_code exit_code;

		list_head link;

		process_dispatcher(const char* name, pid id, pid parent_id, size_t flags)
			: state(PROC_STATE_EMBRYO), id(id), parent_id(parent_id),
			  runs(0), kstack(0), mm(nullptr), flags(flags), wating_state(PROC_WAITING_NONE),
			  exit_code(ERROR_SUCCESS)
		{

			memset(&this->trapframe, 0, sizeof(this->trapframe));

			memmove(this->name, name, std::min((size_t)strlen(name), PROC_NAME_LEN));
		}
	};

	enum binary_types
	{
		BINARY_ELF
	};

	void process_init(void);

// create a process
	error_code create_process(IN const char* name,
		IN size_t flags,
		IN bool inherit_parent,
		OUT process_dispatcher** ret);

	error_code process_load_binary(IN process_dispatcher* porc,
		IN uint8_t* bin,
		IN size_t binsize,
		IN binary_types type);

	error_code process_run(IN process_dispatcher* porc);

	// handle process cleanup when exiting
	void process_exit(IN process_dispatcher* proc);

	// terminate current process
	error_code process_terminate(error_code error_code);

	// terminate the given process
	error_code process_terminate(pid pid, error_code error_code);

	// update context information of current process
	// update from trap frame
	error_code process_update_context(trap::trap_frame tf);
	// update from syscall
	error_code process_update_context(syscall::syscall_regs regs);

} // namespace process

extern __thread process::process_dispatcher* current;
