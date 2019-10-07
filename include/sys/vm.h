#if !defined(__INCLUDE_SYS_VM_H)
#define __INCLUDE_SYS_VM_H
#include "boot/multiboot2.h"
#include "sys/types.h"

namespace vm
{
using pde_t = size_t;
using pte_t = size_t;

void kvm_switch(pde_t *kpml4t);
pde_t *kvm_setup(void);
void kvm_freevm(pde_t *pgdir);
void kvm_init(size_t entrycnt, multiboot_mmap_entry entries[]);
} // namespace vm

#endif // __INCLUDE_SYS_VM_H
