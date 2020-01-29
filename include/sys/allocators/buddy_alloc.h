#if !defined(__INCLUDE_SYS_BUDDY_ALLOC_H)
#define __INCLUDE_SYS_BUDDY_ALLOC_H

#include "sys/types.h"

namespace allocators
{

namespace buddy_allocator
{
void buddy_init(void *as, void *ae);

void *buddy_alloc_4k_page(void);
void buddy_free_4k_page(void *ptr);

void *buddy_alloc_with_order(size_t order);
void buddy_free_with_order(void *ptr, size_t order);

void *buddy_alloc(size_t sz);
void buddy_free(void *m);
} // namespace buddy_allocator

} // namespace allocators

#endif // __INCLUDE_SYS_BUDDY_ALLOC_H
