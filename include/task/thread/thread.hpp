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

#include "drivers/acpi/cpu.h"
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
	friend class scheduler;

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

	struct current
	{
		static void exit(error_code code);
	};

	using trampoline_type = void (*)();

	using routine_type = error_code (*)(void* arg);

	static void default_trampoline();
	static_assert(ktl::Convertible<decltype(default_trampoline), trampoline_type>);

	[[nodiscard]]static error_code_with_result<thread*> create(ktl::shared_ptr<process> parent,
		ktl::string_view name,
		routine_type routine,
		trampoline_type trampoline);

 public:
	thread(const thread&) = delete;
	thread& operator=(const thread&) = delete;

	[[nodiscard]]vmm::mm_struct* get_mm();

 private:
	thread(ktl::shared_ptr<process> parent, ktl::string_view name);

	void remove_from_lists();

	void finish_dying();

	[[noreturn]] void switch_to();

	ktl::shared_ptr<process> parent{ nullptr };

	ktl::unique_ptr<kernel_stack> kstack{ nullptr };

	kbl::doubly_linked_node_state<thread> thread_link{};

	ktl::string_view name{ "" };

	thread_states state{ thread_states::INITIAL };

	error_code exit_code{ ERROR_SUCCESS };

	bool need_reschedule{ false };

	bool critical{ false };

	uint64_t flag{ 0 };

 public:
	using thread_list_type = kbl::intrusive_doubly_linked_list<thread, &thread::thread_link>;

};

class kernel_stack final
{
 public:
	friend class thread;
	friend class scheduler;

	static constexpr size_t MAX_SIZE = 4_MB;
	static constexpr size_t MAX_PAGE_COUNT = MAX_SIZE / PAGE_SIZE;

 public:
	[[nodiscard]] static ktl::unique_ptr<kernel_stack> create(thread::routine_type start_routine,
		void* arg,
		thread::trampoline_type tpl);

	~kernel_stack();

	[[nodiscard]] void* get_raw() const
	{
		return top;
	}

	[[nodiscard]] uintptr_t get_address() const
	{
		return reinterpret_cast<uintptr_t >(get_raw());
	}
 private:
	[[nodiscard]] kernel_stack(thread::routine_type routine, void* arg, thread::trampoline_type tpl);

	void* top{ nullptr };

	trap::trap_frame* tf{ nullptr };

	arch_task_context_registers* context{ nullptr };
};

extern thread::thread_list_type global_thread_list;
extern cls_item<thread*, CLS_CUR_THREAD_PTR> cur_thread;

}