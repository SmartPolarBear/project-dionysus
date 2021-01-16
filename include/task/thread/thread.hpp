#pragma once

#include "object/dispatcher.hpp"

#include "debug/nullability.hpp"
#include "debug/thread_annotations.hpp"

#include "kbl/lock/spinlock.h"
#include "kbl/atomic/atomic.hpp"
#include "kbl/data/list_base.hpp"

#include "ktl/shared_ptr.hpp"
#include "ktl/unique_ptr.hpp"
#include "ktl/string_view.hpp"
#include "ktl/list.hpp"
#include "ktl/concepts.hpp"

#include "system/cls.hpp"

#include "drivers/apic/traps.h"

namespace task
{

extern lock::spinlock global_thread_lock;

class process;

class kernel_stack;

class thread final
	: object::dispatcher<thread, 0>
{
 public:
	friend class process;

	friend
	class scheduler;

	enum class [[clang::enum_extensibility(closed)]] thread_states
	{
		INITIAL,
		READY,
		RUNNING,
		SUSPENDED,
		DYING,
		DEAD,
	};

	enum [[clang::flag_enum, clang::enum_extensibility(open)]] thread_flags : uint64_t
	{
		FLAG_DETACHED,
		FLAG_IDLE,
		FLAG_DEFERRED_FREE,
	};

	enum [[clang::flag_enum, clang::enum_extensibility(open)]] thread_signals : uint64_t
	{
		SIGNAL_KILLED
	};

	struct current
	{
		static void exit(error_code code);
	};

	using trampoline_type = void (*)();

	using routine_type = error_code (*)(void* arg);

	static void default_trampoline();

	static_assert(ktl::Convertible<decltype(default_trampoline), trampoline_type>);

	[[nodiscard]]static error_code_with_result<task::thread*> create(ktl::shared_ptr<process> parent,
		ktl::string_view name,
		routine_type routine,
		void* arg,
		trampoline_type trampoline = default_trampoline);

	[[nodiscard]]static error_code_with_result<task::thread*> create_and_enqueue(ktl::shared_ptr<process> parent,
		ktl::string_view name,
		routine_type routine,
		void* arg,
		trampoline_type trampoline = default_trampoline);

	[[nodiscard]]static error_code create_idle();
 public:
	thread(const thread&) = delete;
	thread& operator=(const thread&) = delete;

	[[nodiscard]]vmm::mm_struct* get_mm();

	[[nodiscard]]bool get_need_reschedule() const
	{
		return need_reschedule;
	}

	void kill();

	void resume();

	void suspend();

	error_code detach();

	/// \brief wait until the thread complete
	/// \param out_err_code the exit code of the thread
	/// \return whether the joining is succeeded.
	error_code join(error_code* out_err_code);

	/// \brief forcibly remove the thread from global thread list and free the thread
	[[deprecated("We rarely force terminate a thread using this.")]] void forget();
 private:
	[[noreturn]]static error_code idle_routine(void* arg);
	static_assert(ktl::Convertible<decltype(idle_routine), routine_type>);

	thread(ktl::shared_ptr<process> parent, ktl::string_view name);

	void remove_from_lists();

	void finish_dying();

	void switch_to() TA_REQ(global_thread_lock);

	ktl::shared_ptr<process> parent{ nullptr };

	ktl::unique_ptr<kernel_stack> kstack{ nullptr };

	kbl::doubly_linked_node_state<thread> thread_link{};

	ktl::string_view name{ "" };

	thread_states state{ thread_states::INITIAL };

	error_code exit_code{ ERROR_SUCCESS };

	bool need_reschedule{ false };

	bool critical{ false };

	uint64_t flags{ 0 };

	uint64_t signals{ 0 };

 public:
	using thread_list_type = kbl::intrusive_doubly_linked_list<thread, &thread::thread_link>;
};

class kernel_stack final
{
 public:
	friend class thread;

	friend
	class scheduler;

	static constexpr size_t MAX_SIZE = 4_MB;
	static constexpr size_t MAX_PAGE_COUNT = MAX_SIZE / PAGE_SIZE;

 public:
	[[nodiscard]] static ktl::unique_ptr<kernel_stack> create(
		thread::routine_type start_routine,
		void* arg,
		thread::trampoline_type tpl);

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
	[[nodiscard]] kernel_stack(void* stk_mem, thread::routine_type routine, void* arg, thread::trampoline_type tpl);

	void* bottom{ nullptr };

	trap::trap_frame* tf{ nullptr };

	arch_task_context_registers* context{ nullptr };

	uintptr_t top{ 0 };
};

extern thread::thread_list_type global_thread_list;
extern cls_item<thread*, CLS_CUR_THREAD_PTR> cur_thread;

}