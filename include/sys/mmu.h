/**
 * @ Author: SmartPolarBear
 * @ Create Time: 2019-09-22 13:11:10
 * @ Modified by: SmartPolarBear
 * @ Modified time: 2019-10-02 16:23:37
 * @ Description:
 */

#if !defined(__INCLUDE_SYS_MMU_H)
#define __INCLUDE_SYS_MMU_H

#if !defined(__cplusplus)
#define PML4_SIZE (0x1000)
#define PML4_ALIGN (0x1000)
#define PML4_ENTRY_SIZE (8)
#define PML4_ADDR2ENTRYID(addr) (((addr) >> 39) & 0x1FF)

#define PDPT_SIZE (0x1000)
#define PDPT_ALIGN (0x1000)
#define PDPT_ENTRY_SIZE (8)
#define PDPT_ADDR2ENTRYID(addr) (((addr) >> 30) & 0x1FF)

#define PD_SIZE (0x1000)
#define PD_ALIGN (0x1000)
#define PD_ENTRY_SIZE (8)

#define PT_SIZE (0x1000)
#define PT_ALIGN (0x1000)
#define PT_ENTRY_SIZE (8)

#define PG_P (1 << 0)
#define PG_W (1 << 1)
#define PG_U (1 << 2)
#define PG_2MB (1 << 7)
#else

#include "sys/types.h"

constexpr size_t PGSIZE = 4096;

static inline constexpr size_t PGROUNDUP(size_t sz)
{
    return (((sz) + ((size_t)PGSIZE - 1ul)) & ~((size_t)(PGSIZE - 1ul)));
}

#endif //__cplusplus

#endif // __INCLUDE_SYS_MMU_H
