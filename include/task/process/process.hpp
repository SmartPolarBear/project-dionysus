#pragma once
#include "debug/thread_annotations.hpp"

#include "object/dispatcher.hpp"

#include "ktl/span.hpp"
#include "ktl/list.hpp"
#include "ktl/string_view.hpp"
#include "ktl/shared_ptr.hpp"

#include "kbl/atomic/atomic.hpp"

#include "system/scheduler.h"

#include "drivers/acpi/cpu.h"
#include "drivers/apic/traps.h"

extern cls_item<task::process*, CLS_PROC_STRUCT_PTR> cur_proc;

namespace task
{

void process_init();

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

class job;
class user_stack;

class process final
	: public object::dispatcher<process, 0>
{
 public:
	static constexpr size_t PROC_MAX_NAME_LEN = 64;

	static constexpr size_t KSTACK_PAGES = 2;
	static constexpr size_t KSTACK_SIZE = KSTACK_PAGES * PAGE_SIZE;

	[[maybe_unused]]static constexpr size_t USTACK_PAGES_PER_THREAD = 8;
	[[maybe_unused]]static constexpr size_t USTACK_GUARD_PAGES_PER_THREAD = 1;
	[[maybe_unused]]static constexpr size_t
		USTACK_TOTAL_PAGES_PER_THREAD = (USTACK_GUARD_PAGES_PER_THREAD + USTACK_PAGES_PER_THREAD);

	[[maybe_unused]]static constexpr size_t USTACK_USABLE_SIZE_PER_THREAD = USTACK_PAGES_PER_THREAD * PAGE_SIZE;
	[[maybe_unused]]static constexpr size_t USTACK_TOTAL_SIZE = USTACK_TOTAL_PAGES_PER_THREAD * PAGE_SIZE;

	static constexpr size_t USTACK_FREELIST_THRESHOLD = 16;

	enum class [[clang::enum_extensibility(closed)]] Status
	{
		INITIAL,
		RUNNING,
		DYING,
		DEAD,
	};

 public:
	friend class job;
	friend class thread;

	static kbl::integral_atomic<pid_type> pid_counter;
	static_assert(kbl::integral_atomic<pid_type>::is_always_lock_free);

	static error_code_with_result<ktl::shared_ptr<process>> create(const char* name,
		const ktl::shared_ptr<job>& parent);

	static error_code_with_result<ktl::shared_ptr<task::process>> create(const char* name,
		void* bin,
		size_t size,
		ktl::shared_ptr<job> parent);

	process() = delete;
	process(const process&) = delete;

	~process() override = default;

	[[maybe_unused]] void exit(task_return_code code)  noexcept;
	void kill(task_return_code code) noexcept;
	void finish_dead_transition() noexcept;

	void remove_thread(thread* t);

	[[nodiscard]] ktl::string_view get_name() const
	{
		return name.data();
	}

	[[nodiscard]] process_state get_state() const
	{
		return state;
	}

	void set_state(process_state _state)
	{
		state = _state;
	}

	[[nodiscard]] pid_type get_id() const
	{
		return id;
	}

	void set_id(pid_type _id)
	{
		id = _id;
	}

	// FIXME: remove first
	vmm::mm_struct* get_mm() const
	{
		return mm;
	}

	size_t get_flags() const
	{
		return flags;
	}

	void set_flags(size_t _flags)
	{
		flags = _flags;
	}

	/// \brief Get status of this process
	/// \return the status
	[[nodiscard]] Status get_status() const
	{
		return status;
	}

	error_code resize_heap(IN OUT uintptr_t* heap_ptr);

 private:

	[[nodiscard]] error_code_with_result<void*> make_next_user_stack() TA_REQ(lock);

	[[nodiscard]] process(std::span<char> name,
		pid_type id,
		ktl::shared_ptr<job> parent,
		ktl::shared_ptr<job> critical_to);

	/// \brief  status transition, must be called with held lock
	/// \param st the new status
	void set_status_locked(Status st) noexcept TA_REQ(lock);

	void kill_all_threads_locked() noexcept TA_REQ(lock, !global_thread_lock);

	void add_child_thread(thread* t) noexcept TA_EXCL(lock);

	[[nodiscard]] error_code_with_result<user_stack*> allocate_ustack(thread* t) TA_EXCL(lock);
	void free_ustack(user_stack* ustack) TA_EXCL(lock);

	error_code setup_mm() TA_REQ(lock);

	Status status;

	ktl::shared_ptr<job> parent;
	ktl::shared_ptr<job> critical_to;
	bool kill_critical_when_nonzero_code{ false };

	char _name_buf[PROC_NAME_LEN]{};
	ktl::span<char> name;

	ktl::list<thread*> threads TA_GUARDED(lock){};

	ktl::list<user_stack*> free_list TA_GUARDED(lock){};
	ktl::list<user_stack*> busy_list TA_GUARDED(lock){};

	task_return_code ret_code;

	process_state state;

	pid_type id;

	pid_type parent_id;

	size_t runs;

	vmm::mm_struct* mm;

	size_t flags;

};

}