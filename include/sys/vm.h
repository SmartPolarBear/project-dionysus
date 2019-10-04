#if !defined(__INCLUDE_SYS_VM_H)
#define __INCLUDE_SYS_VM_H
#include "boot/multiboot2.h"
#include "sys/types.h"
namespace vm
{
using pml4_t = size_t;
using pdpt_t = size_t;
using pgdir_t = size_t;
using pde_t = size_t;

void kvm_switch(void);
void kvm_setup(size_t entrycnt, multiboot_mmap_entry entries[]);
} // namespace vm

#endif // __INCLUDE_SYS_VM_H
