#if !defined(__INCLUDE_SYS_VMM_H)
#define __INCLUDE_SYS_VMM_H

#include "sys/types.h"

namespace vmm
{
using pde_t = size_t;
using pde_ptr_t = pde_t *;

// initialize the vmm
// 1) register the page fault handle
// 2) allocate an pml4 table
void init_vmm(void);

// When called by pmm, first map [0,2GiB] to [KERNEL_VIRTUALBASE,KERNEL_VIRTUALEND]
void boot_map_kernel_mem(void);

// install GDT
void install_gdt(void);
} // namespace vmm

#endif // __INCLUDE_SYS_VMM_H
