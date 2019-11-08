#if !defined(__INCLUDE_SYS_BUDDY_ALLOC_H)
#define __INCLUDE_SYS_BUDDY_ALLOC_H

#include "sys/types.h"

namespace vm
{
void buddy_init(void *as, void *ae);

void *buddy_alloc(size_t sz);
void buddy_free(void *m);
} // namespace vm

#endif // __INCLUDE_SYS_BUDDY_ALLOC_H
