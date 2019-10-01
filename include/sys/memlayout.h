/**
 * @ Author: SmartPolarBear
 * @ Create Time: 2019-09-22 13:11:14
 * @ Modified by: SmartPolarBear
 * @ Modified time: 2019-10-01 19:34:18
 * @ Description:
 */
#if !defined(__INCLUDE_SYS_MEMLAYOUT_H)
#define __INCLUDE_SYS_MEMLAYOUT_H

#if !defined(__cplusplus)

#define KPHYSICAL (0x0000000000400000)
#define KVIRTUAL (0xFFFFFFFF80400000)

#else

#include "sys/types.h"

constexpr uint64_t KERNEL_VIRTUALBASE = 0xFFFFFFFF80000000;
constexpr uint64_t KERNEL_VIRTUALLINK = 0xFFFFFFFF80400000;

template <typename P>
static inline constexpr P *V2P(P *a) { return (P *)(((uint64_t)(a)) - KERNEL_VIRTUALBASE); }

template <typename P>
static inline constexpr P *P2V(P *a) { return (P *)(((void *)(((char *)(a)) + KERNEL_VIRTUALBASE))); }

#endif

// #if !defined(__cplusplus)

// #define EXTMEM 0x100000     // Start of extended memory
// #define DEVSPACE 0xFE000000 // Other devices are at high addresses

// // Key addresses for address space layout (see kmap in vm.c for layout)
// #define KERNBASE 0x80000000          // First kernel virtual address
// #define KERNLINK (KERNBASE + EXTMEM) // Address where kernel is linked

// #define V2P(a) (((uint)(a)) - KERNBASE)
// #define P2V(a) ((void *)(((char *)(a)) + KERNBASE))

// #define V2P_WO(x) ((x)-KERNBASE)   // same as V2P, but without casts
// #define P2V_WO(x) ((x) + KERNBASE) // same as P2V, but without casts

// #else
// #include "sys/types.h"

// constexpr size_t EXTMEM = 0x100000;
// constexpr size_t DEVSPACE = 0xFE000000;

// constexpr size_t KERNBASE = 0x80000000;
// constexpr size_t KERNLINK(void) { return KERNBASE + EXTMEM; }

// constexpr void *V2P(void *a) { return (void *)(((uint)(a)) - KERNBASE); }
// constexpr void *P2V(void *a) { return (void *)(((void *)(((char *)(a)) + KERNBASE))); }

// template <typename P>
// constexpr P *V2P(P *a) { return (P *)(((uint)(a)) - KERNBASE); }

// template <typename P>
// constexpr P *P2V(P *a) { return (P *)(((void *)(((char *)(a)) + KERNBASE))); }

// #endif //__cplusplus

#endif // __INCLUDE_SYS_MEMLAYOUT_H
