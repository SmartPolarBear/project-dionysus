#pragma once
// #if !defined(__INCLUDE_DRIVERS_ACPI_CPU_H)
// #define __INCLUDE_DRIVERS_ACPI_CPU_H

#include "sys/mmu.h"
#include "sys/types.h"


struct cpu
{
    uint8_t id;                // index into cpus[] below
    uint8_t apicid;            // Local APIC ID
    context *scheduler;        // swtch() here to enter scheduler
    volatile uint32_t started; // Has the CPU started?
    int nest_pushcli_depth;    // Depth of pushcli nesting.
    int intr_enable;           // Were interrupts enabled before pushcli?
    bool present;              // Is this core available
    void *local;               // Cpu-local storage variables

    // get tss from cpu local storage
    uint32_t *get_tss()
    {
        if (local == nullptr)
        {
            return nullptr;
        }

        uint8_t *local_storage=reinterpret_cast<decltype(local_storage)>(local);
        uint32_t *tss = reinterpret_cast<decltype(tss)>(local_storage + 1024);
        
        return tss;
    }
};

using cpu_info = struct cpu;

constexpr size_t CPU_COUNT_LIMIT = 8;

extern cpu_info cpus[CPU_COUNT_LIMIT];
extern uint8_t cpu_count;

extern __thread cpu_info *cpu;

namespace ap
{
void init_ap(void);
void all_processor_main();
} // namespace ap

// #endif // __INCLUDE_DRIVERS_ACPI_CPU_H
