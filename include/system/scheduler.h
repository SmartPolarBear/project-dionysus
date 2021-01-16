#pragma once

namespace scheduler
{
	[[noreturn, clang::optnone]]void scheduler_loop();
	void scheduler_yield();
	void scheduler_enter();
} // namespace context
