#pragma once

#include "object/dispatcher.hpp"

#include "debug/nullability.hpp"
#include "debug/thread_annotations.hpp"

#include "kbl/lock/spinlock.h"
#include "kbl/atomic/atomic.hpp"
#include "kbl/data/list_base.hpp"

#include "ktl/shared_ptr.hpp"
#include "ktl/string_view.hpp"
#include "ktl/list.hpp"
#include "ktl/concepts.hpp"

#include "system/cls.hpp"

#include "drivers/acpi/cpu.h"

namespace task
{

class process;

class kernel_stack;

class thread final
	: object::dispatcher<thread, 0>
{
 public:
	friend class process;

	enum class [[clang::enum_extensibility(closed)]] thread_states
	{
		INITIAL,
		READY,
		RUNNING,
		SUSPENDED,
		DYING,
		DEAD,
	};

	using trampoline_type = void (*)();

	using routine_type = error_code (*)(void* arg);

	static void trampoline();

	static_assert(ktl::Convertible<decltype(trampoline), trampoline_type>);

	[[nodiscard]]static thread* create(ktl::shared_ptr<process> parent,
		ktl::string_view name,
		routine_type routine,
		trampoline_type trampoline);

 public:
	[[nodiscard]]vmm::mm_struct* get_mm();

 private:

	[[noreturn]] void switch_to();

	ktl::shared_ptr<process> parent{ nullptr };

	ktl::shared_ptr<kernel_stack> kstack{ nullptr };

	arch_task_context_registers context{};

	kbl::doubly_linked_node_state<thread> thread_link{};

	ktl::string_view name{ "" };

	thread_states state{ thread_states::INITIAL };

	error_code exit_code{ ERROR_SUCCESS };

};

class kernel_stack final
{
 public:
	friend class thread;

	static constexpr size_t MAX_SIZE = 16_MB;
	static constexpr size_t MAX_PAGE_COUNT = MAX_SIZE / PAGE_SIZE;

 public:
	[[nodiscard]] ktl::shared_ptr<kernel_stack> create(vmm::mm_struct* parent_mm, thread::trampoline_type tpl);

	[[nodiscard]] void* get_raw() const
	{
		return top;
	}

	[[nodiscard]] uintptr_t get_address() const
	{
		return reinterpret_cast<uintptr_t >(get_raw());
	}
 private:
	error_code grow_downward(size_t count = 1);

	void* top;
	vmm::mm_struct* mm;
};

}