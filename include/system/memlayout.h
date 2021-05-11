#pragma once
#include "system/mmu.h"

#include "ktl/concepts.hpp"

#if !defined(__cplusplus)
#error "This header is only for C++"
#endif //__cplusplus

#include "system/types.h"

#include "kbl/data/pod_list.h"

// the end of the kernel binary
extern uint8_t end[]; // kernel.ld

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

constexpr uintptr_t USER_TOP = 0x00007fffffffffff;

static inline constexpr bool VALID_KERNEL_PTR(uintptr_t ptr)
{
	return KERNEL_VIRTUALBASE <= ptr && ptr <= KERNEL_VIRTUALEND;
}

static inline constexpr bool VALID_USER_PTR(uintptr_t ptr)
{
	return ptr <= USER_TOP;
}

static inline constexpr bool VALID_USER_PTR(ktl::convertible_to<uintptr_t> auto ptr)
{
	auto addr = static_cast<uintptr_t>(ptr);
	return addr <= USER_TOP;
}

static inline constexpr bool VALID_USER_PTR(ktl::is_pointer auto ptr)
{
	auto addr = reinterpret_cast<uintptr_t>(ptr);
	return addr <= USER_TOP;
}



constexpr bool VALID_KERNEL_REGION(uintptr_t start, uintptr_t end)
{
	return (KERNEL_VIRTUALBASE < start) && (start < end) && (end <= KERNEL_VIRTUALEND);
}

constexpr bool VALID_USER_REGION(uintptr_t start, uintptr_t end)
{
	return start < end && end <= USER_TOP;
}

// leave a page guard hole
constexpr uintptr_t USER_STACK_TOP = USER_TOP - PAGE_SIZE;

// for memory-mapped IO
// TODO: dynamically map for memory-mapped IO
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

template<typename P>
static inline P V2P_WO(P a)
{
	return (P)(V2P((uintptr_t)a));
}

template<typename P>
static inline P P2V_WO(P a)
{
	return (P)(P2V((uintptr_t)a));
}

template<typename P>
static inline P IO2V_WO(P a)
{
	return (P)(IO2V((uintptr_t)a));
}

template<typename P>
static inline P V2P(void* a)
{
	return (P)(V2P((uintptr_t)a));
}

template<typename P>
static inline P P2V(void* a)
{
	return (P)(P2V((uintptr_t)a));
}

template<typename P>
static inline P IO2V(void* a)
{
	return (P)(IO2V((uintptr_t)a));
}
