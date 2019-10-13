/**
 * @ Author: SmartPolarBear
 * @ Create Time: 2019-09-22 13:11:10
 * @ Modified by: SmartPolarBear
 * @ Modified time: 2019-10-13 22:46:57
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

//Unused: we use 2MB page
#define PT_SIZE (0x1000)
#define PT_ALIGN (0x1000)
#define PT_ENTRY_SIZE (8)

#define PG_P (1 << 0)
#define PG_W (1 << 1)
#define PG_U (1 << 2)
#define PG_2MB (1 << 7)
#else

#include "sys/types.h"

constexpr size_t PML4T_SIZE = 0x1000;
constexpr size_t PML4T_ALIGN = 0x1000;
constexpr size_t PML4T_ENTRY_SIZE = 8;

constexpr size_t PDPT_SIZE = 0x1000;
constexpr size_t PDPT_ALIGN = 0x1000;
constexpr size_t PDPT_ENTRY_SIZE = 8;

constexpr size_t PD_SIZE = 0x1000;
constexpr size_t PD_ALIGN = 0x1000;
constexpr size_t PD_ENTRY_SIZE = 8;

constexpr size_t PDENTRIES_COUNT = 512;

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

constexpr size_t PD_SHIFT = 22;
#endif //__cplusplus

#endif // __INCLUDE_SYS_MMU_H
