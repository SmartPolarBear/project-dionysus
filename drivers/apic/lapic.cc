#include "drivers/acpi/cpu.h"
#include "drivers/apic/apic.h"
#include "drivers/apic/traps.h"
#include "drivers/debug/kdebug.h"

#include "arch/amd64/x86.h"

volatile uint32_t *local_apic::lapic;


void local_apic::write_lapic(size_t index, uint32_t value)
{
    local_apic::lapic[index] = value;
    [[maybe_unused]] auto val = local_apic::lapic[local_apic::ID]; // wait for finish by reading
}

void local_apic::init_lapic(void)
{
    if (!lapic)
    {
        KDEBUG_GENERALPANIC("LAPIC isn't avaiable.\n");
    }

    //enable local APIC
    write_lapic(SVR, ENABLE | (TRAP_IRQ0 + IRQ_SPURIOUS));

    // disbale logical interrupt lines
    write_lapic(LINT0, INTERRUPT_MASKED);
    write_lapic(LINT1, INTERRUPT_MASKED);

    // Disable performance counter overflow interrupts
    // on machines that provide that interrupt entry.
    if (((lapic[VER] >> 16) & 0xFF) >= 4)
    {
        write_lapic(PCINT, INTERRUPT_MASKED);
    }

    // Map error interrupt to IRQ_ERROR.
    write_lapic(ERROR, TRAP_IRQ0 + IRQ_ERROR);

    // Clear error status register (requires back-to-back writes).
    write_lapic(ESR, 0);
    write_lapic(ESR, 0);

    // Ack any outstanding interrupts.
    write_lapic(EOI, 0);

    // Send an Init Level De-Assert to synchronise arbitration ID's.
    write_lapic(ICRHI, 0);
    write_lapic(ICRLO, ICRLO_CMD_BCAST | ICRLO_CMD_INIT | ICRLO_CMD_LEVEL);
    while (lapic[ICRLO] & ICRLO_CMD_DELIVS)
        ;

    // Enable interrupts on the APIC (but not on the processor).
    write_lapic(TASK_PRIORITY, 0);
}

// when interrupts are enable
// it can be dangerous to call this, getting short-lasting results
// NOTICE: this can cause even tripple fault
//          can be highly relavant to lock aquire and release
size_t local_apic::get_cpunum(void)
{
    if (read_eflags() & EFLAG_IF)
    {
        KDEBUG_GENERALPANIC_WITH_RETURN_ADDR("local_apic::get_cpunum can't be called with interrupts enabled\n");
    }

    if (lapic == nullptr)
    {
        return 0;
    }

    auto id = lapic[ID] >> 24;
    for (size_t i = 0; i < cpu_count; i++)
    {
        if (id == cpu[i].apicid)
        {
            return i;
        }
    }

    return 0;
}
