#pragma once

#include "object/dispatcher.hpp"

#include "debug/nullability.hpp"
#include "debug/thread_annotations.hpp"

#include "kbl/lock/spinlock.h"
#include "kbl/data/list.hpp"
#include "kbl/data/name.hpp"
#include "kbl/checker/canary.hpp"

#include "ktl/shared_ptr.hpp"
#include "ktl/unique_ptr.hpp"
#include "ktl/string_view.hpp"
#include "ktl/concepts.hpp"
#include "kbl/lock/lock_guard.hpp"

#include "system/cls.hpp"
#include "system/time.hpp"
#include "system/deadline.hpp"

#include "syscall_handles.hpp"

#include "drivers/apic/traps.h"

#include "task/thread/wait_queue.hpp"
#include "task/thread/cpu_affinity.hpp"
#include "task/thread/user_stack.hpp"

#include "task/scheduler/scheduler_config.hpp"

#include "task/ipc/message.hpp"

#include <compare>

#ifndef USE_SCHEDULER_CLASS
#error "USE_SCHEDULER_CLASS MUST BE DEFINED"
#endif

namespace task
{

using context = arch_task_context_registers;

using thread_trampoline_type = void (*)();

using thread_routine_type = error_code (*)(void* arg);

extern lock::spinlock global_thread_lock;

class kernel_stack final
{
 public:
	friend class thread;
	friend class process;

	friend class scheduler;

	static constexpr size_t MAX_SIZE = 4_MB;
	static constexpr size_t MAX_PAGE_COUNT = MAX_SIZE / PAGE_SIZE;

 public:
	[[nodiscard]] static ktl::unique_ptr<kernel_stack> create(thread* parent,
		thread_routine_type start_routine,
		void* arg,
		thread_trampoline_type tpl);

	~kernel_stack();

	[[nodiscard]] void* get_raw() const
	{
		return bottom;
	}

	[[nodiscard]] uintptr_t get_address() const
	{
		return reinterpret_cast<uintptr_t >(get_raw());
	}
 private:
	[[nodiscard]] kernel_stack(thread* parent_thread,
		void* stk_mem,
		thread_routine_type routine,
		void* arg,
		thread_trampoline_type tpl);

	thread* parent{ nullptr };

	void* bottom{ nullptr };

	trap::trap_frame* tf{ nullptr };

	task::context* context{ nullptr };
};

class wait_queue_state
{
 public:
	friend class wait_queue;
	friend class thread;

	wait_queue_state() = delete;
	wait_queue_state(const wait_queue_state&) = delete;
	wait_queue_state& operator=(const wait_queue_state&) = delete;

	explicit wait_queue_state(thread* pa)
		: parent_(pa)
	{
	}

	~wait_queue_state();

	bool holding() TA_REQ(global_thread_lock);

	void block(wait_queue::interruptible intr, error_code status) TA_REQ(global_thread_lock);

	void unblock_if_interruptible(thread* t, error_code status) TA_REQ(global_thread_lock);

	bool unsleep(thread* thread, error_code status) TA_REQ(global_thread_lock);
	bool unsleep_if_interruptible(thread* thread, error_code status) TA_REQ(global_thread_lock);

 private:

	thread* parent_{ nullptr };

	wait_queue* blocking_on_{ nullptr };
	error_code block_code_{ ERROR_SUCCESS };
	wait_queue::interruptible interruptible_{ wait_queue::interruptible::No };
};

class task_state
{
 public:
	task_state() = default;

	void init(thread_routine_type entry, void* arg);

	[[nodiscard]] thread_routine_type get_routine()
	{
		return routine_;
	}

	[[nodiscard]] void* get_arg()
	{
		return arg_;
	}

	error_code get_exit_code() const
	{
		return exit_code_;
	}
	void set_exit_code(error_code exit_code)
	{
		exit_code_ = exit_code;
	}

	error_code join(const deadline&) TA_REQ(global_thread_lock);
	void wake_joiners(error_code status) TA_REQ(global_thread_lock);

 private:
	thread_routine_type routine_{ nullptr };

	void* arg_{ nullptr };

	error_code exit_code_{ ERROR_SUCCESS };

	wait_queue exit_code_wait_queue_{};
};

class ipc_state
{
 public:
	static constexpr size_t MR_SIZE = 64;
	static constexpr size_t BR_SIZE = 33;

	friend class wait_queue;
	friend class thread;

	friend error_code (::sys_ipc_load_message(const syscall::syscall_regs* regs));
	friend error_code (::sys_ipc_store(const syscall::syscall_regs* regs));

	enum [[clang::enum_extensibility(closed)]] states
	{
		IPC_RECEIVING,
		IPC_SENDING,
		IPC_FREE,
	};

	ipc_state() = delete;
	explicit ipc_state(thread* parent) : parent_(parent)
	{
	}
	ipc_state(const ipc_state&) = delete;
	ipc_state& operator=(const ipc_state&) = delete;

	error_code receive_locked(thread* from, const deadline& ddl) TA_REQ(global_thread_lock);

	error_code send_locked(thread* to, const deadline& ddl) TA_REQ(global_thread_lock);

	template<typename T>
	T get_typed_item(size_t index)
	{
		return *reinterpret_cast<T*>(mr_ + index);
	}

	ipc::message_register_type get_mr(size_t index)
	{
		return mr_[index];
	}

	void set_mr(size_t index, ipc::message_register_type value)
	{
		mr_[index] = value;
	}

	ipc::message_register_type get_br(size_t index)
	{
		return br_[index];
	}

	void set_br(size_t index, ipc::message_register_type value)
	{
		br_[index] = value;
	}

	void load_mrs_locked(size_t start, ktl::span<ipc::message_register_type> mrs);

	void store_mrs_locked(size_t st, ktl::span<ipc::message_register_type> mrs);

	void copy_mrs_to_locked(thread* another, size_t st, size_t cnt);

	[[nodiscard]] ipc::message_tag get_message_tag();

	[[nodiscard]] ipc::message_acceptor get_acceptor();

	/// \brief set the message tag to mrs. will reset mr_count_, which influence exist items
	/// \param tag
	void set_message_tag_locked(const ipc::message_tag* tag) noexcept;

	/// \brief set acceptor to brs. will reset mr_count_, which influence exist items
	/// \param acc
	void set_acceptor(const ipc::message_acceptor* acc) noexcept;

 private:
	/// \brief handle extended items like strings and map/grant items
	/// \param to
	/// \return
	error_code send_extended_items(thread* to);

	/// \brief message registers
	ipc::message_register_type mr_[MR_SIZE]{ 0 };

	size_t mr_count_{ 0 };

	/// \brief buffer registers
	ipc::buffer_register_type br_[BR_SIZE]{ 0 };

	size_t br_count_{ 0 };

	thread* parent_{ nullptr };

	states state_{ IPC_FREE };

	wait_queue sender_wait_queue_{};

	wait_queue receiver_wait_queue_{};

	mutable lock::spinlock lock_{ "ipc_lock" };
};

class scheduler_state
	: public SCHEDULER_STATE_BASE
{
 public:
	friend class thread;

	scheduler_state() = delete;
	scheduler_state(const scheduler_state&) = delete;
	scheduler_state& operator=(const scheduler_state&) = delete;

	~scheduler_state()
	{
	}

	explicit scheduler_state(thread* parent) : parent_{ parent }
	{

	}

	[[nodiscard]] bool need_reschedule() const
	{
		return need_reschedule_;
	}

	void set_need_reschedule(bool need)
	{
		need_reschedule_ = need;
	}

	[[nodiscard]] cpu_affinity* affinity()
	{
		return &affinity_;
	}

 private:
	thread* parent_{ nullptr };

	cpu_affinity affinity_{ CPU_NUM_INVALID, cpu_affinity_type::SOFT };
	bool need_reschedule_{ false };
};

class thread final
	: public object::solo_dispatcher<thread, 0>
{
 public:
	friend class process;

	friend class scheduler;
	friend class scheduler_class;

	friend class USE_SCHEDULER_CLASS;

	friend class kernel_stack;
	friend class user_stack;
	friend class wait_queue;
	friend class wait_queue_state;
	friend class ipc_state;

	friend struct wait_queue_list_node_trait;

	friend class process_user_stack_state;

	friend error_code (::sys_ipc_load_message(const syscall::syscall_regs* regs));
	friend error_code (::sys_ipc_send(const syscall::syscall_regs* regs));
	friend error_code (::sys_ipc_receive(const syscall::syscall_regs* regs));
	friend error_code (::sys_ipc_store(const syscall::syscall_regs* regs));

	enum class [[clang::enum_extensibility(closed)]] thread_states
	{
		INITIAL,
		READY,
		RUNNING,
		SUSPENDED,
		BLOCKED,
		BLOCKED_READ_LOCK,
		SLEEPING,
		DYING,
		DEAD,
	};

	enum [[clang::flag_enum, clang::enum_extensibility(open)]] thread_flags : uint64_t
	{
		FLAG_DETACHED = 0b1,
		FLAG_IDLE = 0b10,
		FLAG_INIT = 0b100,
		FLAG_DEFERRED_FREE = 0b1000,
	};

	enum [[clang::flag_enum, clang::enum_extensibility(open)]] thread_signals : uint64_t
	{
		SIGNAL_KILLED,
		SIGNAL_SUSPEND
	};

	struct current
	{
		static void exit(error_code code);
	};

	using link_type = kbl::list_link<thread, lock::spinlock>;

	static void default_trampoline();

	static_assert(ktl::convertible_to<decltype(default_trampoline), thread_trampoline_type>);

	[[nodiscard]]static error_code_with_result<task::thread*> create(process* parent,
		ktl::string_view name,
		thread_routine_type routine,
		void* arg,
		thread_trampoline_type trampoline = default_trampoline,
		cpu_affinity aff = cpu_affinity{ CPU_NUM_INVALID, cpu_affinity_type::SOFT });

	[[nodiscard]]static error_code create_idle();
 public:
	~thread() override;

	thread(const thread&) = delete;
	thread& operator=(const thread&) = delete;

	object::object_type get_type() const override
	{
		return object::object_type::THREAD;
	}

	[[nodiscard]] vmm::mm_struct* get_mm();

	[[nodiscard]] bool is_user_thread() const
	{
		return parent_ != nullptr;
	}

	[[nodiscard]] uint64_t get_flags() const
	{
		return flags_;
	}

	void set_flags(uint64_t fl)
	{
		flags_ = fl;
	}

	void kill() TA_REQ(!global_thread_lock);
	void kill_locked() TA_REQ(global_thread_lock);

	void resume();

	error_code suspend();

	error_code detach();

	error_code detach_and_resume();

	/// \brief wait until the thread complete
	/// \param out_err_code the exit code of the thread
	/// \return whether the joining is succeeded.
	error_code join(error_code* out_err_code);

	error_code join(error_code* out_err_code, deadline deadline);

	/// \brief forcibly remove the thread from global thread list and free the thread
	[[deprecated("We rarely force terminate a thread using this."), maybe_unused]]
	void forget() TA_REQ(!global_thread_lock);

	[[nodiscard]] ktl::string_view get_name() const
	{
		return name_.data();
	}

	[[nodiscard]]const char* get_name_raw() const
	{
		return name_.data();
	}

	bool is_idle() const
	{
		return flags_ & FLAG_IDLE;
	}

	[[nodiscard]] ipc_state* get_ipc_state()
	{
		return &ipc_state_;
	}

	[[nodiscard]] scheduler_state* get_scheduler_state()
	{
		return &scheduler_state_;
	}

	thread_states state{ thread_states::INITIAL };

 private:
	void switch_to(interrupt_saved_state_type state_to_restore) TA_REQ(global_thread_lock);

	thread(process* parent, ktl::string_view name, cpu_affinity affinity);

	void remove_from_lists() TA_REQ(global_thread_lock);

	void finish_dead_transition();

	void process_pending_signals();

	kbl::canary<kbl::magic("thrd")> canary_;

	kbl::name<64> name_{ "" };

	ktl::unique_ptr<kernel_stack> kstack_{ nullptr };

	user_stack* ustack_{ nullptr };

	process* parent_{ nullptr };

	bool critical_{ false };

	uint64_t flags_{ 0 };

	uint64_t signals_{ 0 };

	task_state task_state_{};

	wait_queue_state wait_queue_state_{ this };

	ipc_state ipc_state_{ this };

	scheduler_state scheduler_state_{ this };

	link_type run_queue_link{ this };
	link_type zombie_queue_link{ this };
	link_type wait_queue_link{ this };
	link_type master_list_link{ this };
	link_type process_link{ this };
 public:
	using master_list_type = kbl::intrusive_list_with_default_trait<thread,
	                                                                lock::spinlock,
	                                                                &thread::master_list_link,
	                                                                true>;

	using process_list_type = kbl::intrusive_list_with_default_trait<thread,
	                                                                 lock::spinlock,
	                                                                 &thread::process_link,
	                                                                 true>;
};

extern thread::master_list_type global_thread_list;

extern cls_item<thread*, CLS_CUR_THREAD_PTR> cur_thread;

}