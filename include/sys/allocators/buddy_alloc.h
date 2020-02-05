#if !defined(__INCLUDE_SYS_BUDDY_ALLOC_H)
#define __INCLUDE_SYS_BUDDY_ALLOC_H

#include "sys/memlayout.h"
#include "sys/mmu.h"
#include "sys/pmm.h"
#include "sys/types.h"

namespace allocators
{

namespace buddy_allocator
{

void init_buddy(void);
void buddy_init_memmap(page_info *base, size_t n);
page_info *buddy_alloc_pages(size_t n);
void buddy_free_pages(page_info *base, size_t n);
size_t buddy_get_free_pages_count(void);

extern pmm::pmm_manager_info buddy_pmm_manager;

} // namespace buddy_allocator

} // namespace allocators

#endif // __INCLUDE_SYS_BUDDY_ALLOC_H
