#pragma once

#include "drivers/debug/kdebug.h"
#include "drivers/lock/spinlock.h"
#include "drivers/apic/traps.h"

error_code default_trap_handle([[maybe_unused]] trap::trap_frame info);

struct handle_table_struct; // traps.cc
extern handle_table_struct handle_table; // traps.cc