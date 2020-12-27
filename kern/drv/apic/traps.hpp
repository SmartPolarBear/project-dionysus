#pragma once

#include "debug/kdebug.h"
#include "kbl/lock/spinlock.h"
#include "drivers/apic/traps.h"

error_code default_trap_handle([[maybe_unused]] trap::trap_frame tf);
error_code msi_base_trap_handle([[maybe_unused]]trap::trap_frame tf);

struct handle_table_struct; // traps.cc

extern handle_table_struct handle_table; // traps.cc
