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

struct tss
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
