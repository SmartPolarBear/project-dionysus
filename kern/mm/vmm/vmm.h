#if !defined(__KERN_MM_VMM_H)
#define __KERN_MM_VMM_H

#include "arch/amd64/x86.h"

#include "sys/error.h"
#include "sys/vmm.h"

// paging.cc
extern vmm::pde_ptr_t g_kpml4t;

// page_fualt.cc
extern error_code handle_pgfault([[maybe_unused]] trap_frame info);

//pgdir_cache.cc
void pgdir_cache_init();
vmm::pde_ptr_t pgdir_entry_alloc();
void pgdir_entry_free(vmm::pde_ptr_t entry);

#endif // __KERN_MM_VMM_H
