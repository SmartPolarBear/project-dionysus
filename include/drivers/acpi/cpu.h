#pragma once
// #if !defined(__INCLUDE_DRIVERS_ACPI_CPU_H)
// #define __INCLUDE_DRIVERS_ACPI_CPU_H

#include "arch/amd64/cpu.h"

#include "sys/mmu.h"
#include "sys/types.h"

#include "sys/segmentation.hpp"

struct cpu_struct
{
    uint8_t id;                // index into cpus[] below
    uint8_t apicid;            // Local APIC ID
    volatile uint32_t started; // Has the CPU started?
    int nest_pushcli_depth;    // Depth of pushcli nesting.
    int intr_enable;           // Were interrupts enabled before pushcli?
    bool present;              // Is this core available

    void *local_fs; // Cpu-local storage variables
    void *local_gs;

    task_state_segment tss;
    gdt_table gdt_table;

    cpu_struct()
        : id(0), apicid(0),
          started(0), nest_pushcli_depth(0),
          intr_enable(0), present(false),
          local_fs(nullptr)
    {
        gdt_table = {
            {0, 0, 0, 0x00, 0x00, 0}, /* 0x00 null  */
            {0, 0, 0, 0x9a, 0xa0, 0}, /* 0x08 kernel code (kernel base selector) */
            {0, 0, 0, 0x92, 0xa0, 0}, /* 0x10 kernel data */
            {0, 0, 0, 0x00, 0x00, 0}, /* 0x18 null (user base selector) */
            {0, 0, 0, 0x92, 0xa0, 0}, /* 0x20 user data */
            {0, 0, 0, 0x9a, 0xa0, 0}, /* 0x28 user code */
            {0, 0, 0, 0x92, 0xa0, 0}, /* 0x30 ovmf data */
            {0, 0, 0, 0x9a, 0xa0, 0}, /* 0x38 ovmf code */
            {0, 0, 0, 0x89, 0xa0, 0}, /* 0x40 tss low */
            {0, 0, 0, 0x00, 0x00, 0}, /* 0x48 tss high */
        };
    }

    void install_gdt_and_tss()
    {
        gdt_table_ptr gdt_ptr = {sizeof(gdt_table) - 1, (uintptr_t)&gdt_table};
        load_gdt(&gdt_ptr);
        // lgdt(&gdt_ptr);
        // ltr(0x40);
    }
};

constexpr size_t CPU_COUNT_LIMIT = 8;

extern cpu_struct cpus[CPU_COUNT_LIMIT];
extern uint8_t cpu_count;

extern __thread cpu_struct *cpu;

namespace ap
{
void init_ap(void);
void all_processor_main();
} // namespace ap

// #endif // __INCLUDE_DRIVERS_ACPI_CPU_H
