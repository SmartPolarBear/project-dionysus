#pragma once
#include "debug/thread_annotations.hpp"

#include "object/dispatcher.hpp"

#include "ktl/span.hpp"
#include "ktl/list.hpp"
#include "ktl/string_view.hpp"
#include "ktl/shared_ptr.hpp"
#include "ktl/weak_ptr.hpp"
#include "ktl/atomic.hpp"

#include "object/handle_table.hpp"

#include "system/scheduler.h"

#include "drivers/acpi/cpu.h"
#include "drivers/apic/traps.h"

#include "task/thread/user_stack.hpp"

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

enum process_flags
{
	PROC_EXITING = 0b1,
	PROC_SYS_SERVER = 0b10,
	PROC_DRIVER = 0b100,
	PROC_USER = 0b1000,
};

class process_user_stack_state
{
 public:
	[[maybe_unused]]static constexpr size_t USTACK_PAGES_PER_THREAD = 8;
	[[maybe_unused]]static constexpr size_t USTACK_GUARD_PAGES_PER_THREAD = 1;
	[[maybe_unused]]static constexpr size_t
		USTACK_TOTAL_PAGES_PER_THREAD = (USTACK_GUARD_PAGES_PER_THREAD + USTACK_PAGES_PER_THREAD);

	[[maybe_unused]]static constexpr size_t USTACK_USABLE_SIZE_PER_THREAD = USTACK_PAGES_PER_THREAD * PAGE_SIZE;
	[[maybe_unused]]static constexpr size_t USTACK_TOTAL_SIZE = USTACK_TOTAL_PAGES_PER_THREAD * PAGE_SIZE;

	static constexpr size_t USTACK_FREELIST_THRESHOLD = 16;

	using list_type = kbl::intrusive_list<user_stack, lock::spinlock, user_stack_list_node_trait, true>;

	process_user_stack_state() = delete;
	process_user_stack_state(const process_user_stack_state&) = delete;
	process_user_stack_state& operator=(const process_user_stack_state&) = delete;

	explicit process_user_stack_state(process* par) : parent_(par)
	{
	}

	~process_user_stack_state();

	[[nodiscard]] error_code_with_result<user_stack*> allocate_ustack(thread* t);
	void free_ustack(user_stack* ustack);

 private:
	[[nodiscard]] error_code_with_result<void*> make_next_user_stack_locked() TA_REQ(lock_);

	process* parent_;

	list_type free_list_ TA_GUARDED(lock_){};
	list_type busy_list_ TA_GUARDED(lock_){};

	mutable lock::spinlock lock_;
};

class job;

class process final
	: public object::solo_dispatcher<process, 0>
{
 public:
	using link_type = kbl::list_link<process, lock::spinlock>;

	static constexpr size_t PROC_MAX_NAME_LEN = 64;

	static constexpr size_t KSTACK_PAGES = 2;
	static constexpr size_t KSTACK_SIZE = KSTACK_PAGES * PAGE_SIZE;

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
	friend class process_user_stack_state;

	friend error_code (::sys_get_current_process(const syscall::syscall_regs* regs));
	friend error_code (::sys_get_current_thread(const syscall::syscall_regs* regs));

	static error_code_with_result<process*> create(const char* name,
		const ktl::shared_ptr<job>& parent);

	static error_code_with_result<process*> create(const char* name,
		void* bin,
		size_t size,
		const ktl::shared_ptr<job>& parent);

	process() = delete;
	process(const process&) = delete;

	~process() override;

	object::object_type get_type() const override
	{
		return object::object_type::PROCESS;
	}

	[[maybe_unused]] void exit(task_return_code code) noexcept;
	void kill(task_return_code code) noexcept;
	void finish_dead_transition() noexcept;

	void remove_thread(thread* t);

	error_code suspend();
	void resume();

	[[nodiscard]] ktl::string_view get_name() const
	{
		return name_.data();
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
		return status_;
	}

	error_code resize_heap(IN OUT uintptr_t* heap_ptr);

	[[nodiscard]]object::handle_table* handle_table()
	{
		return &handle_table_;
	}

 private:
	[[nodiscard]] process(std::span<char> name,
		const ktl::shared_ptr<job>& parent,
		const ktl::shared_ptr<job>& critical_to);

	/// \brief  status transition, must be called with held lock
	/// \param st the new status
	void set_status_locked(Status st) noexcept TA_REQ(lock);

	void kill_all_threads_locked() noexcept TA_REQ(lock, !global_thread_lock);

	void add_child_thread(thread* t) noexcept TA_EXCL(lock);

	error_code setup_mm() TA_REQ(lock);

	Status status_;

	ktl::weak_ptr<job> parent_;
	ktl::weak_ptr<job> critical_to_;

	bool kill_critical_when_nonzero_code_{ false };

	kbl::canary<kbl::magic("proc")> canary_;

	kbl::name<PROC_MAX_NAME_LEN> name_;

	thread::process_list_type threads_ TA_GUARDED(lock){};

	process_user_stack_state user_stack_state_{ this };

	int64_t suspend_count_ TA_GUARDED(lock) { 0 };

	task_return_code ret_code_;

	vmm::mm_struct* mm;

	size_t flags;

	object::handle_table handle_table_{ this };

	link_type job_link{ this };
};

}