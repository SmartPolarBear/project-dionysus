#if !defined(__KERN_MM_VMM_H)
#define __KERN_MM_VMM_H

#include "arch/amd64/x86.h"

#include "sys/error.h"
#include "sys/vmm.h"

#include "drivers/apic/traps.h"

// paging.cc
extern vmm::pde_ptr_t g_kpml4t;

// page_fualt.cc
extern error_code handle_pgfault([[maybe_unused]] trap::trap_frame info);

// pgdir_cache.ccc
void pgdir_cache_init();
// others are defined in sys/vmm.h

#endif // __KERN_MM_VMM_H
