#pragma once

#include "object/dispatcher.hpp"

#include "debug/nullability.hpp"
#include "debug/thread_annotations.hpp"

#include "kbl/lock/spinlock.h"
#include "kbl/atomic/atomic.hpp"
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

#include "drivers/apic/traps.h"

#include "task/thread/wait_queue.hpp"

#include <compare>

namespace task
{

using context = arch_task_context_registers;

using thread_trampoline_type = void (*)();

using thread_routine_type = error_code (*)(void* arg);

extern lock::spinlock global_thread_lock;

class process;
class thread;

class kernel_stack final
{
 public:
	friend class thread;
	friend class process;

	friend
	class scheduler;

	static constexpr size_t MAX_SIZE = 4_MB;
	static constexpr size_t MAX_PAGE_COUNT = MAX_SIZE / PAGE_SIZE;

 public:
	[[nodiscard]] static kernel_stack* create(thread* parent,
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

class user_stack
{
 public:
	friend class process;
	friend class thread;

 public:

	user_stack() = delete;
	user_stack(const user_stack&) = delete;
	user_stack& operator=(const user_stack&) = delete;

	[[nodiscard]] void* get_top();

	int64_t operator<=>(const user_stack& rhs) const
	{
		return (uint8_t*)this->top - (uint8_t*)rhs.top;
	}

 private:
	user_stack(process* p, thread* t, void* stack_ptr);

	void* top{ nullptr };

	process* owner_process{ nullptr };
	thread* owner_thread{ nullptr };
};

enum class [[clang::enum_extensibility(closed)]] cpu_affinity_type
{
	SOFT, HARD
};

struct cpu_affinity final
{
	cpu_num_type cpu;
	cpu_affinity_type type;

	auto operator<=>(const cpu_affinity&) const = default;
};

class thread final
	: object::dispatcher<thread, 0>
{
 public:
	friend class process;

	friend class scheduler;
	friend class scheduler_class_base;
	friend class round_rubin_scheduler_class;

	friend class kernel_stack;
	friend class user_stack;
	friend class wait_queue;
	friend class wait_queue_state;

	friend struct wait_queue_list_node_trait;

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

	[[nodiscard]] vmm::mm_struct* get_mm();

	[[nodiscard]] bool get_need_reschedule() const
	{
		return need_reschedule_;
	}

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

	const char* get_name_raw() const
	{
		return name_.data();
	}

	bool is_idle() const
	{
		return flags_ & FLAG_IDLE;
	}

	thread_states state{ thread_states::INITIAL };

 private:
	void switch_to() TA_REQ(global_thread_lock);

	thread(process* parent, ktl::string_view name, cpu_affinity affinity);

	void remove_from_lists() TA_REQ(global_thread_lock);

	void finish_dead_transition();

	void process_pending_signals();

	kbl::canary<kbl::magic("thrd")> canary_;

	kbl::name<64> name_{ "" };

	error_code exit_code_{ ERROR_SUCCESS };

	wait_queue* exit_code_wait_queue_{ nullptr };

	wait_queue_state* wait_queue_state_{ nullptr };

	kernel_stack* kstack_{ nullptr };

	user_stack* ustack_{ nullptr };

	process* parent_{ nullptr };

	bool need_reschedule_{ false };

	bool critical_{ false };

	cpu_affinity affinity_{ CPU_NUM_INVALID, cpu_affinity_type::SOFT };

	uint64_t flags_{ 0 };

	uint64_t signals_{ 0 };

	link_type run_queue_link{ this };
	link_type zombie_queue_link{ this };
	link_type wait_queue_link{ this };
	link_type master_list_link{ this };

 public:
	using master_list_type = kbl::intrusive_list_with_default_trait<thread,
	                                                                lock::spinlock,
	                                                                &thread::master_list_link,
	                                                                true>;
};

extern thread::master_list_type global_thread_list;

extern cls_item<thread*, CLS_CUR_THREAD_PTR> cur_thread;

}