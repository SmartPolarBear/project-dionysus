#pragma once

#include "arch/amd64/regs.h"

#include "sys/mmu.h"
#include "sys/types.h"
#include "sys/vmm.h"

#include "lib/libc/string.h"
#include "lib/libcxx/algorithm"

namespace process
{
enum process_state
{
	PROC_STATE_UNUSED,
	PROC_STATE_EMBRYO,
	PROC_STATE_SLEEPING,
	PROC_STATE_RUNNABLE,
	PROC_STATE_RUNNING,
	PROC_STATE_ZOMBIE
};

using pid = int64_t;

constexpr size_t PROC_NAME_LEN = 32;

// it should be enough
constexpr size_t PROC_MAX_COUNT = INT32_MAX;

constexpr pid PID_MAX = INT64_MAX;

// Per-process state
struct process_dispatcher
{
	static constexpr size_t KERNSTACK_PAGES = 8;
	static constexpr size_t KERNSTACK_SIZE = KERNSTACK_PAGES * PMM_PAGE_SIZE;

	char name[PROC_NAME_LEN + 1];

	process_state state;

	pid id;
	pid parent_id;

	size_t run_times;
	uintptr_t kstack;

	vmm::mm_struct *mm;

	trap_frame trapframe;
	uintptr_t pgdir_addr;

	size_t flags;

	list_head link;

	process_dispatcher(const char *name, pid id, pid parent_id, size_t flags)
		: flags(flags), id(id), parent_id(parent_id), state(PROC_STATE_EMBRYO),
		  run_times(0), kstack(0), mm(nullptr)
	{

		memset(&proc->trapframe, 0, sizeof(proc->trapframe));
		
		strncpy(this->name, name, sysstd::min((size_t)strlen(name), PROC_NAME_LEN));
	}
};

enum process_flags
{
	PROC_SYS_SERVER,
	PROC_USER,
	PROC_DRIVER,
};

enum binary_types
{
	BINARY_ELF
};

void process_init(void);

// create a process
error_code create_process(IN const char *name,
						  IN size_t flags,
						  IN bool inherit_parent,
						  OUT process_dispatcher *proc);

error_code process_load_binary(IN process_dispatcher *porc,
							   IN uint8_t *bin,
							   IN size_t binsize,
							   IN binary_types type);

error_code process_run(IN process_dispatcher *porc);

} // namespace process
