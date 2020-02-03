

#if !defined(__INCLUDE_SYS_BOOTMM_H)
#define __INCLUDE_SYS_BOOTMM_H

#include "sys/types.h"

namespace allocators
{

namespace boot_allocator
{
// FIXME: this value will cause fault on VBOX if it is larger than 32, but now we enlarged our
// page size, so we should enlarge this as well
constexpr uintptr_t BOOT_MEM_LIMIT = 64_MB;
constexpr size_t BOOTMM_BLOCKSIZE = PHYSICAL_PAGE_SIZE;

void bootmm_init(void *vstart, void *vend);
void bootmm_free(char *v);
size_t bootmm_get_used(void);
char *bootmm_alloc(void);
} // namespace boot_allocator

} // namespace allocators

#endif // __INCLUDE_SYS_BOOTMM_H
