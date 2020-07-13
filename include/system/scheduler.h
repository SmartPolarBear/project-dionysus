#pragma once

namespace scheduler
{
	[[clang::optnone]] void scheduler_halt();
	[[clang::optnone]] void scheduler_yield();
	[[clang::optnone]] void scheduler_enter();
	[[noreturn]] [[clang::optnone]] void scheduler_thread();
} // namespace scheduler
