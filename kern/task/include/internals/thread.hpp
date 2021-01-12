#pragma once

#include "drivers/apic/traps.h"
#include "drivers/acpi/cpu.h"

extern "C" [[noreturn]] void user_proc_entry();

extern "C" [[noreturn]] void context_switch(context** from, context* to);