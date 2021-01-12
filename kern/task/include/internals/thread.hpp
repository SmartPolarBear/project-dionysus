#pragma once

#include "drivers/apic/traps.h"
#include "drivers/acpi/cpu.h"

extern "C" [[noreturn, clang::optnone]] void user_proc_entry();

extern "C" [[clang::optnone]]void context_switch(context**, context*);