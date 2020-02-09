/**
 * @ Author: SmartPolarBear
 * @ Create Time: 2019-09-22 13:11:10
 * @ Modified by: Daniel Lin
 * @ Modified time: 2020-01-03 23:19:16
 * @ Description:
 */

#if !defined(__INCLUDE_SYS_MMU_H)
#define __INCLUDE_SYS_MMU_H

#if !defined(__cplusplus) && !defined(__ASSEMBLER__)
#error "This header is only for C++"
#endif //__cplusplus

#ifdef __ASSEMBLER__
// this is for ap_boot.S
// so they must be macros

#define SEG_NULLASM \
    .word 0, 0;     \
    .byte 0, 0, 0, 0

// The 0xC0 means the limit is in 4096-byte units
// and (for executable segments) 32-bit mode.
#define SEG_ASM(type, base, lim)                    \
    .word(((lim) >> 12) & 0xffff), ((base)&0xffff); \
    .byte(((base) >> 16) & 0xff), (0x90 | (type)),  \
        (0xC0 | (((lim) >> 28) & 0xf)), (((base) >> 24) & 0xff)

#define STA_X 0x8 // Executable segment
#define STA_E 0x4 // Expand down (non-executable segments)
#define STA_C 0x4 // Conforming code segment (executable only)
#define STA_W 0x2 // Writeable (non-executable segments)
#define STA_R 0x2 // Readable (executable segments)
#define STA_A 0x1 // Accessed

#else
// not in ap_boot.S

#include "sys/types.h"

constexpr size_t PDENTRIES_COUNT = 512;
constexpr size_t PTENTRIES_COUNT = 512;

constexpr bool PG_PS_ENABLE = true;
constexpr size_t PG_SIZE = 4_KB;
constexpr size_t PG_PS_SIZE = 2_MB;

constexpr size_t PHYSICAL_PAGE_SIZE = PG_PS_ENABLE ? PG_PS_SIZE : PG_SIZE;
constexpr size_t PMM_PAGE_SIZE = 4_KB;

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

enum SegmentIndex
{
    SEG_KCODE = 1,
    SEG_KDATA = 2,
    SEG_KCPU = 3,
    SEG_UCODE = 4,
    SEG_UDATA = 5,
    SEG_TSS = 6,
};

// Task state segment format
struct machine_state
{
    uint32_t link; // Old ts selector
    uint32_t esp0; // Stack pointers and segment selectors
    uint16_t ss0;  //   after an increase in privilege level
    uint16_t padding1;
    uint32_t *esp1;
    uint16_t ss1;
    uint16_t padding2;
    uint32_t *esp2;
    uint16_t ss2;
    uint16_t padding3;
    void *cr3;     // Page directory base
    uint32_t *eip; // Saved state from last task switch
    uint32_t eflags;
    uint32_t eax; // More saved state (registers)
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t *esp;
    uint32_t *ebp;
    uint32_t esi;
    uint32_t edi;
    uint16_t es; // Even more saved state (segment selectors)
    uint16_t padding4;
    uint16_t cs;
    uint16_t padding5;
    uint16_t ss;
    uint16_t padding6;
    uint16_t ds;
    uint16_t padding7;
    uint16_t fs;
    uint16_t padding8;
    uint16_t gs;
    uint16_t padding9;
    uint16_t ldt;
    uint16_t padding10;
    uint16_t t;    // Trap on task switch
    uint16_t iomb; // I/O map base address
};

// Segment Descriptor
struct gdt_segment
{
    uint32_t lim_15_0 : 16;  // Low bits of segment limit
    uint32_t base_15_0 : 16; // Low bits of segment base address
    uint32_t base_23_16 : 8; // Middle bits of segment base address
    uint32_t type : 4;       // Segment type (see STS_ constants)
    uint32_t s : 1;          // 0 = system, 1 = application
    uint32_t dpl : 2;        // Descriptor Privilege Level
    uint32_t p : 1;          // Present
    uint32_t lim_19_16 : 4;  // High bits of segment limit
    uint32_t avl : 1;        // Unused (available for software use)
    uint32_t rsv1 : 1;       // Reserved
    uint32_t db : 1;         // 0 = 16-bit segment, 1 = 32-bit segment
    uint32_t g : 1;          // Granularity: limit scaled by 4K when set
    uint32_t base_31_24 : 8; // High bits of segment base address
};

// Gate descriptors for interrupts and traps
struct idt_gate
{
    uint32_t off_15_0 : 16;  // low 16 bits of offset in segment
    uint32_t cs : 16;        // code segment selector
    uint32_t args : 5;       // # args, 0 for interrupt/trap gates
    uint32_t rsv1 : 3;       // reserved(should be zero I guess)
    uint32_t type : 4;       // type(STS_{TG,IG32,TG32})
    uint32_t s : 1;          // must be 0 (system)
    uint32_t dpl : 2;        // descriptor(meaning new) privilege level
    uint32_t p : 1;          // Present
    uint32_t off_31_16 : 16; // high bits of offset in segment
};
#endif // __ASSEMBLER__

#endif // __INCLUDE_SYS_MMU_H
