/**
 * @ Author: SmartPolarBear
 * @ Create Time: 2019-09-22 13:11:10
 * @ Modified by: SmartPolarBear
 * @ Modified time: 2019-11-12 23:18:49
 * @ Description:
 */

#if !defined(__INCLUDE_SYS_MMU_H)
#define __INCLUDE_SYS_MMU_H

#if !defined(__cplusplus)
#error "This header is only for C++"
#endif //__cplusplus

#include "sys/types.h"

constexpr size_t PDENTRIES_COUNT = 512;
constexpr size_t PTENTRIES_COUNT = 512;

constexpr size_t PDX_SHIFT = 21;
constexpr size_t PAGE_SHIFT = 12;
constexpr size_t PTX_SHIFT = 12;

constexpr size_t PX_MASK = 0x1FF;

constexpr size_t PAGE_SIZE = 4096;

// Page table/directory entry flags
enum PageEntryFlags
{
    PG_P = 0x001,   // Present
    PG_W = 0x002,   // Writeable
    PG_U = 0x004,   // User
    PG_PWT = 0x008, // Write-Through
    PG_PCD = 0x010, // Cache-Disable
    PG_A = 0x020,   // Accessed
    PG_D = 0x040,   // Dirty
    PG_PS = 0x080,  // Page Size
    PG_MBZ = 0x180, // Bits must be zero
};

enum ExceptionType : uint32_t
{
    IT_TRAP = 0x8F00,
    IT_INTERRUPT = 0x8E00
};

enum DescriptorPrivilegeLevel
{
    DPL_KERNEL = 0x0,
    DPL_USER = 0x3
};

enum Segment
{
    SEG_KCODE = 1,
    SEG_KDATA = 2,
    SEG_KCPU = 3,
    SEG_UCODE = 4,
    SEG_UDATA = 5,
    SEG_TSS = 6,
};

constexpr size_t PGROUNDUP(size_t sz)
{
    return (((sz) + ((size_t)PAGE_SIZE - 1)) & ~((size_t)(PAGE_SIZE - 1)));
}
constexpr size_t PGROUNDDOWN(size_t a)
{
    return (((a)) & ~((size_t)(PAGE_SIZE - 1)));
}

#endif // __INCLUDE_SYS_MMU_H
