#pragma once

#include "drivers/apic/traps.h"
#include "drivers/acpi/cpu.h"

#include "task/thread/thread.hpp"

extern "C"
{

[[noreturn, clang::optnone, maybe_unused]] void user_entry();

[[noreturn, clang::optnone]] void thread_trampoline_s();
[[noreturn, clang::optnone]] void thread_entry();

[[clang::optnone]]void context_switch(task::context**, task::context*);

}
