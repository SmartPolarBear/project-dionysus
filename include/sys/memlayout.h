/**
 * @ Author: SmartPolarBear
 * @ Create Time: 2019-09-22 13:11:14
 * @ Modified by: Daniel Lin
 * @ Modified time: 2020-01-03 23:19:14
 * @ Description:
 */
#if !defined(__INCLUDE_SYS_MEMLAYOUT_H)
#define __INCLUDE_SYS_MEMLAYOUT_H

#if !defined(__cplusplus)
#error "This header is only for C++"
#endif //__cplusplus

#include "sys/types.h"

#include "lib/libkern/data/list.h"

// user's address space limit
constexpr size_t USER_ADDRESS_SPACE_LIMIT = 0x00007fffffffffff;

// kernel's address place begin
constexpr size_t KERNEL_ADDRESS_SPACE_BASE = 0xFFFF800000000000;

// the max value for a valid address
constexpr uintptr_t VIRTUALADDR_LIMIT = 0xFFFFFFFFFFFFFFFF;

// remap of physical memory
constexpr uintptr_t PHYREMAP_VIRTUALBASE = 0xffff888000000000;
constexpr uintptr_t PHYREMAP_VIRTUALEND = 0xffffc87fffffffff;
constexpr size_t PHYMEMORY_SIZE = PHYREMAP_VIRTUALEND - PHYREMAP_VIRTUALBASE + 1;

// map kernel, from physical address 0 to 2GiB
constexpr uintptr_t KERNEL_VIRTUALBASE = 0xFFFFFFFF80000000;
constexpr uintptr_t KERNEL_VIRTUALEND = VIRTUALADDR_LIMIT;
constexpr uintptr_t KERNEL_SIZE = KERNEL_VIRTUALEND - KERNEL_VIRTUALBASE + 1;
// Note: the multiboot info will be placed just after kernel
// Be greatly cautious not to overwrite it !!!!
constexpr uintptr_t KERNEL_VIRTUALLINK = 0xFFFFFFFF80100000;

// for memory-mapped IO
constexpr uintptr_t DEVICE_VIRTUALBASE = 0xFFFFFFFF40000000;
constexpr uintptr_t DEVICE_PHYSICALBASE = 0xFE000000;
// this value can be change if needed.
constexpr size_t DEVICE_SIZE = 32_MB;
constexpr uintptr_t DEVICE_PHYSICALEND = DEVICE_PHYSICALBASE + DEVICE_SIZE;

// convert with uintptr_t, defined in vm_utils.cc
extern uintptr_t V2P(uintptr_t x);
extern uintptr_t P2V(uintptr_t x);
extern uintptr_t P2V_KERNEL(uintptr_t x);
extern uintptr_t P2V_PHYREMAP(uintptr_t x);
extern uintptr_t IO2V(uintptr_t x);

template <typename P>
static inline P V2P_WO(P a)
{
    return (P)(V2P((uintptr_t)a));
}

template <typename P>
static inline P P2V_WO(P a)
{
    return (P)(P2V((uintptr_t)a));
}

template <typename P>
static inline P IO2V_WO(P a)
{
    return (P)(IO2V((uintptr_t)a));
}

template <typename P>
static inline P V2P(void *a)
{
    return (P)(V2P((uintptr_t)a));
}

template <typename P>
static inline P P2V(void *a)
{
    return (P)(P2V((uintptr_t)a));
}

template <typename P>
static inline P IO2V(void *a)
{
    return (P)(IO2V((uintptr_t)a));
}

enum page_info_flags
{
    PHYSICAL_PAGE_FLAG_RESERVED = 0b01,
    PHYSICAL_PAGE_FLAG_PROPERTY = 0b10,
    PHYSICAL_PAGE_FLAG_ACTIVE = 0b100,
    PHYSICAL_PAGE_FLAG_DIRTY = 0b1000,
    PHYSICAL_PAGE_FLAG_SWAP = 0b1000,
};

// descriptor of physical memory pages
struct page_info
{
    size_t ref;
    size_t flags;
    size_t property;
    size_t zone_id;
    list_head page_link;
};

static inline bool page_has_flag(const page_info *pg, page_info_flags fl)
{
    return pg->flags & fl;
}

static inline void page_set_flag(page_info *pg, page_info_flags fl)
{
    pg->flags |= fl;
}

static inline void page_clear_flag(page_info *pg, page_info_flags fl)
{
    pg->flags &= ~fl;
}

struct free_area_info
{
    list_head freelist;
    size_t free_count;
};

#endif // __INCLUDE_SYS_MEMLAYOUT_H
