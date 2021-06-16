#pragma once

#include "arch/amd64/cpu/x86.h"

#include "system/error.hpp"
#include "system/vmm.h"

#include "drivers/apic/traps.h"

// paging.cc
extern vmm::pde_ptr_t g_kpml4t;

// page_fualt.cc
extern error_code handle_pgfault([[maybe_unused]] trap::trap_frame info);

// pgdir_cache.cc
void pgdir_cache_init();


