#pragma once

#include "sys/types.h"

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
