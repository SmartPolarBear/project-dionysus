#pragma once

#include "sys/types.h"

constexpr size_t PDENTRIES_COUNT = 512;
constexpr size_t PTENTRIES_COUNT = 512;

constexpr bool PG_PS_ENABLE = true;
constexpr size_t PG_SIZE = 4_KB;
constexpr size_t PG_PS_SIZE = 2_MB;

constexpr size_t PGTABLE_SIZE = 4_KB;

constexpr size_t PAGE_SIZE = PG_PS_ENABLE ? PG_PS_SIZE : PG_SIZE;

constexpr size_t P4_SHIFT = 39;
constexpr size_t P3_SHIFT = 30;
constexpr size_t P2_SHIFT = 21;   // for 2mb paging, this is the lowest level.
constexpr size_t PX_MASK = 0x1FF; //9bit

static inline constexpr size_t P4X(size_t addr)
{
    return (addr >> P4_SHIFT) & PX_MASK;
}

static inline constexpr size_t P3X(size_t addr)
{
    return (addr >> P3_SHIFT) & PX_MASK;
}

static inline constexpr size_t P2X(size_t addr)
{
    return (addr >> P2_SHIFT) & PX_MASK;
}

static inline constexpr size_t PAGE_ROUNDUP(size_t addr)
{
    return (((addr) + ((size_t)PG_SIZE - 1)) & ~((size_t)(PG_SIZE - 1)));
}

static inline constexpr size_t PAGE_ROUNDDOWN(size_t addr)
{
    return (((addr)) & ~((size_t)(PG_SIZE - 1)));
}

// Page table/directory entry flags
enum pde_flags
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

enum exception_type : uint32_t
{
    IT_TRAP = 0x8F00,
    IT_INTERRUPT = 0x8E00
};

enum dpl_values
{
    DPL_KERNEL = 0x0,
    DPL_USER = 0x3
};

enum SegmentIndex
{
    SEG_KCODE = 1,
    SEG_KDATA = 2,
    SEG_KCPU = 3,
    SEG_UCODE = 4,
    SEG_UDATA = 5,
    SEG_TSS = 6,
};
