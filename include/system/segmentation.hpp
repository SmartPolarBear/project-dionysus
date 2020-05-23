#pragma once

#include "system/concepts.hpp"
#include "system/types.h"

struct gdt_entry
{
    uint16_t limit15_0;
    uint16_t base15_0;
    uint8_t base23_16;
    uint8_t type;
    uint8_t limit19_16_and_flags;
    uint8_t base31_24;
} __attribute__((__packed__));

struct gdt_table_ptr
{
    uint16_t limit;
    uint64_t base;
} __attribute__((__packed__));

struct task_state_segment
{
    uint32_t reserved0;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved1;
    uint64_t ist1;
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iopb_offset;
} __attribute__((__packed__));

struct gdt_table
{
    struct gdt_entry null;
    struct gdt_entry kernel_code;
    struct gdt_entry kernel_data;
    struct gdt_entry null2;
    struct gdt_entry user_data;
    struct gdt_entry user_code;
    struct gdt_entry ovmf_data;
    struct gdt_entry ovmf_code;
    struct gdt_entry tss_low;
    struct gdt_entry tss_high;
} __attribute__((aligned(4096)));

enum GDT_SEGMENTS : uint64_t
{
    SEGMENTSEL_KNULL = 0x00,
    SEGMENTSEL_KCODE = 0x08,
    SEGMENTSEL_KDATA = 0x10,
    SEGMENTSEL_UNULL = 0x18,
    SEGMENTSEL_UDATA = 0x20,
    SEGMENTSEL_UCODE = 0x28,
    SEGMENTSEL_OVMFDATA = 0x30,
    SEGMENTSEL_OVMFCODE = 0x38,
    SEGMENTSEL_TSSLOW = 0x40,
    SEGMENTSEL_TSSHIGH = 0x48
};

// cpu local storage

static_assert(sizeof(uintptr_t) == 0x08);
enum CLS_ADDRESS : uintptr_t
{
    CLS_CPU_STRUCT_PTR = 0,
    CLS_PROC_STRUCT_PTR = 0x8
};

// #define __cls_get(n) ({      \
//     uint64_t res;            \
//     asm("mov %%gs:" #n ",%0" \
//         : "=r"(res));        \
//     res;                     \
// })

// #define __cls_put(n, v) ({ \
//     uint64_t val = v;      \
//     asm("mov %0, %%gs:" #n \
//         :                  \
//         : "r"(val));       \
// })

#pragma clang diagnostic push

// bypass the problem caused by the fault of clang
// that parameters used in inline asm are always reported to be unused

#pragma clang diagnostic ignored "-Wunused-variable"
#pragma clang diagnostic ignored "-Wunused-parameter"

//__attribute__((always_inline))

template <typename T>
 static inline T cls_get(uintptr_t n) requires Pointer<T>
{
    uintptr_t ret = 0;
    asm("mov %%fs:(%%rax),%0"
        : "=r"(ret)
        : "a"(n));
    return (T)(void*)ret;
}

template <typename T>
 static inline void cls_put(uintptr_t n, T v) requires Pointer<T>
{
    uintptr_t val = (uintptr_t)v;      
    asm("mov %0, %%fs:(%%rax)" 
        :                  
        : "r"(val),"a"(n));
}

template <typename T>
static inline T gs_get(uintptr_t n) requires Pointer<T>
{
    uintptr_t ret = 0;
    asm("mov %%gs:(%%rax),%0"
    : "=r"(ret)
    : "a"(n));
    return (T)(void*)ret;
}

template <typename T>
static inline void gs_put(uintptr_t n, T v) requires Pointer<T>
{
    uintptr_t val = (uintptr_t)v;
    asm("mov %0, %%gs:(%%rax)"
    :
    : "r"(val),"a"(n));
}

#pragma clang diagnostic pop
