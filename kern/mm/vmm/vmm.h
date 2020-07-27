#pragma once

#include "arch/amd64/x86.h"

#include "system/error.h"
#include "system/vmm.h"

#include "drivers/apic/traps.h"

// paging.cc
extern vmm::pde_ptr_t g_kpml4t;

// page_fualt.cc
extern error_code handle_pgfault([[maybe_unused]] trap::trap_frame info);

// pgdir_cache.cc
void pgdir_cache_init();

// vma.cc
using vmm::mm_struct;
using vmm::vma_struct;

void check_vma_overlap(vma_struct* prev, vma_struct* next);
void remove_vma(mm_struct* mm, vma_struct* vma);
// others are defined in sys/vmm.h


