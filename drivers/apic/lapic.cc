#include "drivers/apic/apic.h"
#include "drivers/apic/traps.h"
#include "drivers/debug/kdebug.h"

volatile uint32_t *local_apic::lapic;

constexpr size_t ID = 0x0020 / 4;
constexpr size_t VER = 0x0030 / 4;
constexpr size_t TASK_PRIORITY = 0x0080 / 4;
constexpr size_t EOI = 0x00B0 / 4;
constexpr size_t SVR = 0x00F0 / 4; // Spurious Interrupt Vector
enum SVR_FLAGS
{
    ENABLE = 0x00000100
};
constexpr size_t ESR = 0x0280 / 4;
constexpr size_t ICRLO = 0x0300 / 4;
enum ICRLO_COMMAND
{
    ICRLO_CMD_INIT = 0x00000500,    // INIT/RESET
    ICRLO_CMD_STARTUP = 0x00000600, // Startup IPI
    ICRLO_CMD_DELIVS = 0x00001000,  // Delivery status
    ICRLO_CMD_ASSERT = 0x00004000,  // Assert interrupt (vs deassert)
    ICRLO_CMD_DEASSERT = 0x00000000,
    ICRLO_CMD_LEVEL = 0x00008000, // Level triggered
    ICRLO_CMD_BCAST = 0x00080000, // Send to all APICs, including self.
    ICRLO_CMD_BUSY = 0x00001000,
    ICRLO_CMD_FIXED = 0x00000000,
};

constexpr size_t ICRHI = 0x0310 / 4; //Interrupt Command [63:32]
constexpr size_t TIMER = 0x0320 / 4;
enum TIMER_FLAGS
{
    TIMER_FLAG_X1,
    TIMER_FLAG_PERIODIC
};

constexpr size_t PCINT = 0x0340 / 4; // Performance Counter LVT
constexpr size_t LINT0 = 0x0350 / 4; // Local Vector Table 1 (LINT0)
constexpr size_t LINT1 = 0x0360 / 4; // Local Vector Table 2 (LINT1)
constexpr size_t ERROR = 0x0370 / 4; // Local Vector Table 3 (ERROR)
enum INTRRUPT_FLAGS
{
    INTERRUPT_MASKED = 0x00010000
};

constexpr size_t TICR = 0x0380 / 4; // Timer Initial Count
constexpr size_t TCCR = 0x0390 / 4; // Timer Current Count
constexpr size_t TDCR = 0x03E0 / 4; // Timer Divide Configuration
constexpr size_t TIC_DEFUALT_VALUE = 10000000;

static void write_lapic(size_t index, uint32_t value)
{
    local_apic::lapic[index] = value;
    auto val = local_apic::lapic[ID]; // wait for finish by reading
}

void local_apic::init_lapic(void)
{
    if (!lapic)
    {
        KDEBUG_GENERALPANIC("LAPIC isn't avaiable.\n");
    }

    //enable local APIC
    write_lapic(SVR, ENABLE | (TRAP_IRQ0 + IRQ_SPURIOUS));

    // initialize timer
    // TODO: make a seperate timer driver
    write_lapic(TDCR, TIMER_FLAG_X1);
    write_lapic(TIMER, TIMER_FLAG_PERIODIC | (TRAP_IRQ0 + IRQ_TIMER));
    write_lapic(TICR, TIC_DEFUALT_VALUE);

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
