#if !defined(__INCLUDE_DRIVERS_ACPI_CPU_H)
#define __INCLUDE_DRIVERS_ACPI_CPU_H

#include "sys/mmu.h"
#include "sys/types.h"

struct context
{
    uintptr_t r15;
    uintptr_t r14;
    uintptr_t r13;
    uintptr_t r12;
    uintptr_t r11;
    uintptr_t rbx;
    uintptr_t ebp; //rbp
    uintptr_t eip; //rip;
};

struct cpu
{
    uint8_t id;                // index into cpus[] below
    uint8_t apicid;            // Local APIC ID
    context *scheduler;        // swtch() here to enter scheduler
    machine_state ts;              // Used by x86 to find stack for interrupt
    gdt_segment gdt[7];        // x86 global descriptor table
    volatile uint32_t started; // Has the CPU started?
    int ncli;                  // Depth of pushcli nesting.
    int intena;                // Were interrupts enabled before pushcli?

    bool present;

    //Cpu-local storage variables
    void *local;
};

using cpu_info = struct cpu;

constexpr size_t CPU_COUNT_LIMIT = 8;

extern cpu_info cpus[CPU_COUNT_LIMIT];
extern uint8_t cpu_count;

extern __thread cpu_info *cpu;

namespace ap
{
void init_ap(void);
} // namespace ap

#endif // __INCLUDE_DRIVERS_ACPI_CPU_H
