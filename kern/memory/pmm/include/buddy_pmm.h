#pragma once

#include "memory/pmm.hpp"

#include "system/memlayout.h"
#include "system/mmu.h"
#include "system/pmm.h"
#include "system/types.h"

namespace pmm
{

namespace buddy_pmm
{

void init_buddy(void);
void buddy_init_memmap(page *base, size_t n);
page *buddy_alloc_pages(size_t n);
void buddy_free_pages(page *base, size_t n);
size_t buddy_get_free_pages_count(void);

extern pmm::pmm_desc buddy_pmm_manager;

} // namespace buddy_allocator

} // namespace allocators

