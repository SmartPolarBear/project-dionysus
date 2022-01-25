#pragma once

#include "debug/kdebug.h"
#include "kbl/lock/spinlock.h"
#include "drivers/apic/traps.h"

error_code default_trap_handle([[maybe_unused]] trap::trap_frame tf);
error_code msi_base_trap_handle([[maybe_unused]]trap::trap_frame tf);

struct trap_table_struct; // traps.cc

extern trap_table_struct trap_table; // traps.cc
