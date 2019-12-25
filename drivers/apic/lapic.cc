#include "drivers/acpi/cpu.h"
#include "drivers/apic/apic.h"
#include "drivers/apic/traps.h"
#include "drivers/debug/kdebug.h"

#include "sys/memlayout.h"
#include "sys/mmu.h"

#include "arch/amd64/x86.h"

volatile uint32_t *local_apic::lapic;

// Spin for a given number of microseconds.
// On real hardware would want to tune this dynamically.
[[clang::optnone]] static void microdelay(size_t us)
{
    for (size_t i = 0; i < us; i++)
    {
        ; // do nothing. just cost time.
    }
}

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
    return id;

    for (size_t i = 0; i < cpu_count; i++)
    {
        if (id == cpu[i].apicid)
        {
            return i;
        }
    }

    return 0;
}

void local_apic::start_ap(size_t apicid, uintptr_t addr)
{
    [[maybe_unused]] constexpr size_t CMOS_PORT = 0x70;
    [[maybe_unused]] constexpr size_t CMOS_RETURN = 0x71;

    // "The BSP must initialize CMOS shutdown code to 0AH
    // and the warm reset vector (DWORD based at 40:67) to point at
    // the AP startup code prior to the [universal startup algorithm]."
    outb(CMOS_PORT, 0xF); // offset 0xF is shutdown code
    outb(CMOS_PORT + 1, 0x0A);
    uint16_t *wrv = reinterpret_cast<decltype(wrv)>(P2V((0x40 << 4 | 0x67))); // Warm reset vector
    wrv[0] = 0;
    wrv[1] = addr >> 4;

    // "Universal startup algorithm."
    // Send INIT (level-triggered) interrupt to reset other CPU.
    write_lapic(ICRHI, apicid << 24);
    write_lapic(ICRLO, ICRLO_CMD_INIT | ICRLO_CMD_LEVEL | ICRLO_CMD_ASSERT);
    microdelay(200);

    write_lapic(ICRLO, ICRLO_CMD_INIT | ICRLO_CMD_LEVEL);
    microdelay(10000);

    // Send startup IPI (twice!) to enter code.
    // Regular hardware is supposed to only accept a STARTUP
    // when it is in the halted state due to an INIT.  So the second
    // should be ignored, but it is part of the official Intel algorithm.
    for (int i = 0; i < 2; i++)
    {
        write_lapic(ICRHI, apicid << 24);
        write_lapic(ICRLO, ICRLO_CMD_STARTUP | (addr >> 12));
        microdelay(200);
    }
}
