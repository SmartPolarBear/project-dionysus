

#if !defined(__INCLUDE_SYS_BOOTMM_H)
#define __INCLUDE_SYS_BOOTMM_H

#include "sys/types.h"

namespace allocators
{

namespace boot_allocator
{
constexpr size_t BOOTMM_BLOCKSIZE = 4096;

void bootmm_init(void *vstart, void *vend);
void bootmm_free(char *v);
size_t bootmm_get_used(void);
char *bootmm_alloc(void);
} // namespace bootmm

} // namespace allocators

#endif // __INCLUDE_SYS_BOOTMM_H
