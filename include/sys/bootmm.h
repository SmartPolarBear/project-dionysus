

#if !defined(__INCLUDE_SYS_BOOTMM_H)
#define __INCLUDE_SYS_BOOTMM_H

#include "sys/types.h"

namespace vm
{
constexpr size_t BOOTMM_BLOCKSIZE = 4096;

void bootmm_init(void *vstart, void *vend);
void bootmm_free(char *v);
char *bootmm_alloc(void);

} // namespace vm

#endif // __INCLUDE_SYS_BOOTMM_H
