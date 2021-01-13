#pragma once

#include "drivers/apic/traps.h"
#include "drivers/acpi/cpu.h"

extern "C"
{

[[noreturn, clang::optnone]] void user_entry();

[[noreturn, clang::optnone]] void thread_trampoline_s();
[[noreturn, clang::optnone]] void thread_entry();

[[clang::optnone]]void context_switch(context**, context*);

}
