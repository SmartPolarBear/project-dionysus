#pragma once

namespace scheduler
{
	[[noreturn]] [[clang::optnone]] void scheduler_loop();
	[[clang::optnone]] void scheduler_yield();
	[[clang::optnone]] void scheduler_enter();
} // namespace scheduler
