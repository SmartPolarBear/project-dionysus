#if !defined(__KERN_MM_VMM_H)
#define __KERN_MM_VMM_H

#include "arch/amd64/x86.h"

#include "sys/error.h"
#include "sys/vmm.h"

// paging.cc
extern vmm::pde_ptr_t g_kpml4t;

// page_fualt.cc
extern hresult handle_pgfault([[maybe_unused]] trap_info info);

#endif // __KERN_MM_VMM_H
