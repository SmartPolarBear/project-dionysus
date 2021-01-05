#pragma once

#include "object/kernel_object.hpp"

#include "arch/amd64/cpu/regs.h"

#include "system/mmu.h"
#include "system/types.h"
#include "system/vmm.h"
#include "system/syscall.h"
#include "system/messaging.hpp"

#include "drivers/apic/traps.h"
#include "drivers/acpi/cpu.h"
#include "kbl/lock/spinlock.h"

#include "kbl/data/pod_list.h"
#include "kbl/data/list_base.hpp"

#include "task/dispatcher.hpp"

#include <cstring>
#include <algorithm>
#include <span>

namespace task
{
	enum [[clang::enum_extensibility(closed)]]task_return_code : int64_t
	{
		TASK_RETCODE_NORMAL = 0,
		TASK_RETCODE_SYSCALL_KILL,
		TASK_RETCODE_POLICY_KILL,
		TASK_RETCODE_EXCEPTION_KILL,
		TASK_RETCODE_CRITICAL_PROCESS_KILL
	};

	using sleep_channel_type = size_t;
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
		PROC_SYS_SERVER = 0b10,
		PROC_DRIVER = 0b100,
		PROC_USER = 0b1000,
	};

	enum load_binary_flags
	{
		LOAD_BINARY_RUN_IMMEDIATELY = 0b0001
	};

	constexpr size_t PROC_WAITING_INTERRUPTED = 0x80000000;
	enum process_waiting_state : size_t
	{
		PROC_WAITING_NONE,
		PROC_WAITING_CHILD = 0x1ul | PROC_WAITING_INTERRUPTED,
		PROC_WAITING_TIMER = 0x2ul | PROC_WAITING_INTERRUPTED,
	};

	enum binary_types
	{
		BINARY_ELF
	};

	constexpr size_t PROC_NAME_LEN = 64;

// it should be enough
	constexpr size_t PROC_MAX_COUNT = INT32_MAX;

	constexpr pid_type PID_MAX = INT64_MAX;

}