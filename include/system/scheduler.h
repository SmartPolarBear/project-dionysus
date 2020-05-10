#pragma once

namespace scheduler
{
[[clang::optnone]] void scheduler_halt();
[[clang::optnone]] void scheduler_yield();
} // namespace scheduler
