#if !defined(__INCLUDE_SYS_VM_H)
#define __INCLUDE_SYS_VM_H
#include "boot/multiboot2.h"
#include "sys/types.h"

namespace vm
{
// we use 2MB page
using pde_t = size_t;
using pde_ptr_t = pde_t *;

void kvm_switch(pde_t *kpml4t);
pde_t *kvm_setup(void);
void kvm_freevm(pde_t *pgdir);
void kvm_init(void);
} // namespace vm

#endif // __INCLUDE_SYS_VM_H
