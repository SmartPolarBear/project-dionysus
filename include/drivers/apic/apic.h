#if !defined(__INCLUDE_DRIVERS_APIC_H)
#define __INCLUDE_DRIVERS_APIC_H

#include "sys/types.h"

namespace local_apic
{

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

constexpr size_t TICR = 0x0380 / 4;                  // Timer Initial Count
[[maybe_unused]] constexpr size_t TCCR = 0x0390 / 4; // Timer Current Count
constexpr size_t TDCR = 0x03E0 / 4;                  // Timer Divide Configuration
constexpr size_t TIC_DEFUALT_VALUE = 10000000;

extern volatile uint32_t *lapic;

void init_lapic(void);
size_t get_cpunum(void);
void write_lapic(size_t index, uint32_t value);
} // namespace local_apic

namespace io_apic
{
void init_ioapic(void);
void enable_trap(uint32_t trapnum, uint32_t cpu_rounted);
} // namespace io_apic

namespace pic8259A
{

void initialize_pic(void);

} // namespace pic8259A

#endif // __INCLUDE_DRIVERS_APIC_H
