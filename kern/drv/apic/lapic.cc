#include "include/lapic.hpp"

#include "drivers/acpi/cpu.h"
#include "drivers/apic/apic.h"
#include "drivers/apic/traps.h"
#include "drivers/console/console.h"
#include "debug/kdebug.h"
#include "drivers/apic/timer.h"

#include "system/memlayout.h"
#include "system/mmu.h"

#include "arch/amd64/cpu/x86.h"

#include <gsl/util>

using trap::IRQ_ERROR;
using trap::IRQ_SPURIOUS;
using trap::TRAP_IRQ0;

using namespace apic;

// APIC internals
using namespace _internals;

volatile uint32_t* local_apic::lapic;

// Spin for a given number of microseconds.
// On real hardware would want to tune this dynamically.
[[clang::optnone]] static inline void microdelay(size_t count)
{
	for (size_t i = 0; i < count; i++); // do nothing. just consume time.
}

void local_apic::write_lapic(size_t index, uint32_t value)
{
	local_apic::lapic[index] = value;
	[[maybe_unused]] auto val = local_apic::lapic[local_apic::ID]; // wait to finish by reading
}

PANIC void local_apic::init_lapic(void)
{
	if (!lapic)
	{
		KDEBUG_RICHPANIC("LAPIC isn't avaiable.\n", "KERNEL PANIC: LAPIC", false, "");
	}

	// enable local APIC
	// write_lapic(SVR, ENABLE | (trap::IRQ_TO_TRAPNUM(IRQ_SPURIOUS)));
	_internals::svr_reg svr{
		.apic_software_enable=true,
		.vector=trap::IRQ_TO_TRAPNUM(IRQ_SPURIOUS) };
	write_lapic(SVR_ADDR, svr.raw);

	// setup timer
	timer::setup_apic_timer();

	// disbale logical interrupt lines
	lvt_lint_reg lint0{ .mask=true }, lint1{ .mask=1 };
	write_lapic(LINT0_ADDR, lint0.raw);
	write_lapic(LINT1_ADDR, lint1.raw);
//	write_lapic(LINT0, INTERRUPT_MASKED);
//	write_lapic(LINT1, INTERRUPT_MASKED);

	// Disable performance counter overflow interrupts
	// on machines that provide that interrupt entry.
	if (((lapic[VER] >> 16) & 0xFF) >= 4)
	{
		write_lapic(PCINT, INTERRUPT_MASKED);
	}

	// Map error interrupt to IRQ_ERROR.
	lvt_error_reg error_reg{ .vector=trap::IRQ_TO_TRAPNUM(IRQ_ERROR) };
	write_lapic(ERROR_ADDR, error_reg.raw);
//	write_lapic(ERROR, trap::IRQ_TO_TRAPNUM(IRQ_ERROR));

	// Clear error status register (requires back-to-back writes).
	write_lapic(ESR, 0);
	write_lapic(ESR, 0);

	// Ack any outstanding interrupts.
	write_lapic(EOI, 0);

	// Send an Init Level De-Assert to synchronise arbitration ID's.
	write_lapic(ICRHI, 0);
	write_lapic(ICRLO, ICRLO_CMD_BCAST | ICRLO_CMD_INIT | ICRLO_CMD_LEVEL);
	while (lapic[ICRLO] & ICRLO_CMD_DELIVS);

	// Enable interrupts on the APIC (but not on the processor).
	write_lapic(TASK_PRIORITY, 0);
}

// when interrupts are enable
// it can be dangerous to call this, getting short-lasting results
size_t local_apic::get_cpunum()
{
	if (read_eflags() & EFLAG_IF)
	{
		KDEBUG_RICHPANIC("local_apic::get_cpunum can't be called with interrupts enabled\n", "KERNEL PANIC:LAPIC",
			false, "Return address: 0x%x\n", __builtin_return_address(0));
	}

	if (lapic == nullptr)
	{
		return 0;
	}

	auto id = lapic[ID] >> 24;

	for (size_t i = 0; i < cpu_count; i++)
	{
		if (id == cpus[i].apicid)
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
	uint16_t* wrv = reinterpret_cast<decltype(wrv)>(P2V((0x40 << 4 | 0x67))); // Warm reset vector
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

void local_apic::write_eoi()
{
	if (lapic)
	{
		write_lapic(EOI, 0);
	}
}

void local_apic::apic_send_ipi(uint32_t dst_apic_id, delievery_modes mode, uint32_t vec, irq_destinations irq_dest)
{
	lapic_icr icr{};
	icr.vector = vec;
	icr.delivery_mode = mode;

	switch (irq_dest)
	{
	case IRQDST_BROADCAST:
		icr.dest_shorthand = APIC_DEST_SHORTHAND_ALL_BUT_SELF;
		break;

	case IRQDEST_SINGLE:
		icr.dest_mode = DTM_PHYSICAL;
		icr.dest = dst_apic_id;
		break;

	default:
		KDEBUG_GERNERALPANIC_CODE(ERROR_INVALID);
		break;
	}

	icr.level = APIC_LEVEL_ASSERT;
	icr.trigger = TRG_EDGE;

	write_lapic(ICRHI, icr.value_high);
	write_lapic(ICRLO, icr.value_low);
}

void local_apic::apic_send_ipi(uint32_t dst_apic_id, delievery_modes mode, uint32_t vec)
{
	apic_send_ipi(dst_apic_id, mode, vec, IRQDEST_SINGLE);
}

void local_apic::apic_broadcast_ipi(delievery_modes mode, uint32_t vec)
{
	apic_send_ipi(BROADCAST_DST_APIC_ID, mode, vec, IRQDST_BROADCAST);
}
