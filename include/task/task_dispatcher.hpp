#pragma once

#include "object/kernel_object.hpp"

#include "arch/amd64/regs.h"

#include "system/mmu.h"
#include "system/types.h"
#include "system/vmm.h"
#include "system/syscall.h"
#include "system/messaging.hpp"

#include "drivers/apic/traps.h"
#include "drivers/lock/spinlock.h"

#include "kbl/pod_list.h"
#include "kbl/list_base.hpp"

#include <cstring>
#include <algorithm>
#include <span>

namespace task
{
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

	constexpr process_id PID_MAX = INT64_MAX;

	class task_dispatcher
		: public libkernel::single_linked_list_base<task_dispatcher*>
	{
	 public:
		static constexpr size_t KERNSTACK_PAGES = 2;
		static constexpr size_t KERNSTACK_SIZE = KERNSTACK_PAGES * PAGE_SIZE;

		task_dispatcher(std::span<char> name, process_id id, process_id parent_id, size_t flags)
			: single_linked_list_base(), name_length(0), state(PROC_STATE_EMBRYO), id(id), parent_id(parent_id),
			  runs(0), kstack(0), mm(nullptr), flags(flags), wating_state(PROC_WAITING_NONE),
			  exit_code(ERROR_SUCCESS), tf(nullptr), context(nullptr)
		{
			for (auto c:name)
			{
				this->name[name_length++] = c;
			}

			lock::spinlock_initialize_lock(&messaging_data.lock, this->name);
		}

		virtual ~task_dispatcher() = default;

		virtual error_code initialize() = 0;
		virtual error_code setup_mm() = 0;
		virtual error_code load_binary(IN uint8_t* bin,
			[[maybe_unused]] IN size_t binary_size OPTIONAL,
			IN binary_types type,
			IN size_t flags
		) = 0;
		virtual error_code terminate(error_code terminate_error) = 0;
		virtual error_code exit() = 0;

		virtual error_code sleep(sleep_channel_type channel, lock::spinlock* lk) = 0;
		virtual error_code wakeup(sleep_channel_type channel) = 0;
		virtual error_code wakeup_no_lock(sleep_channel_type channel) = 0;
		virtual error_code change_heap_ptr(IN OUT uintptr_t* heap_ptr) = 0;

		[[nodiscard]] process_state get_state() const
		{
			return state;
		}

		void set_state(process_state _state)
		{
			task_dispatcher::state = _state;
		}

		[[nodiscard]] process_id get_id() const
		{
			return id;
		}

		void set_id(process_id _id)
		{
			task_dispatcher::id = _id;
		}

	 protected:

		char name[PROC_NAME_LEN]{};
		size_t name_length;

		process_state state;

		process_id id;

		process_id parent_id;

		size_t runs;

		std::unique_ptr<uint8_t> kstack;

		vmm::mm_struct* mm;

		size_t flags;
		size_t wating_state;
		error_code exit_code;

		trap::trap_frame* tf;
		context* context;

		struct sleep_data_struct
		{
			size_t channel;
		} sleep_data{};

		struct messaging_data_struct
		{
			static constexpr size_t INTERNAL_BUF_SIZE = 64;

			lock::spinlock lock;

			// message passing
			void* data;
			size_t data_size;
			uint8_t internal_buf[INTERNAL_BUF_SIZE];

			// page passing
			bool can_receive;
			void* dst;
			size_t unique_value;
			process_id from;
			uint64_t perms;

		} messaging_data{};
	};
}