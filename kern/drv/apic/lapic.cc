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

volatile uint8_t* local_apic::lapic_base;

// Spin for a given number of microseconds.
// On real hardware would want to tune this dynamically.
[[clang::optnone]] static inline void delay_a_while(size_t count)
{
	for (size_t i = 0; i < count; i++); // do nothing. just consume time.
}

void local_apic::write_lapic(offset_t addr_off, uint32_t value)
{
	auto index = addr_off / sizeof(value);
	((volatile uint32_t*)local_apic::lapic_base)[index] = value;

	((volatile uint32_t*)local_apic::lapic_base)[ID_ADDR / sizeof(uint32_t)]; // wait to finish by reading
}

void local_apic::write_lapic(offset_t addr_off, uint64_t value)
{
	auto index = addr_off / sizeof(value);
	((volatile uint64_t *)local_apic::lapic_base)[index] = value;

	((volatile uint32_t*)local_apic::lapic_base)[ID_ADDR / sizeof(uint32_t)]; // wait to finish by reading
}

PANIC void local_apic::init_lapic()
{
	if (!lapic_base)
	{
		KDEBUG_RICHPANIC("LAPIC isn't avaiable.\n", "KERNEL PANIC: LAPIC", false, "");
	}

	// enable local APIC
	_internals::svr_reg svr{
		.apic_software_enable=true,
		.vector=trap::IRQ_TO_TRAPNUM(IRQ_SPURIOUS) };
	write_lapic(SVR_ADDR, svr);

	// Configure timer
	auto[eax, ebx, ecx, edx]= cpuid(cpuid_requests::CPUID_GETFEATURES);

	if (ecx & features::ecx_bits::CPUID_ECX_BIT_TSCDeadline)
	{
		kdebug::kdebug_log("TSC-Deadline timer mode is supported.\n");
		// TODO: use TSC-Deadline
	}
	else
	{
		kdebug::kdebug_log("TSC-Deadline timer mode is not supported.\n");
	}

	timer_divide_configuration_reg dcr{ .divide_val=TIMER_DIV1 };
	write_lapic(DCR_ADDR, dcr);

	lvt_timer_reg timer_reg{ .vector=(trap::IRQ_TO_TRAPNUM(trap::IRQ_TIMER)), .mask=false, .timer_mode=TIMER_PERIODIC };
	write_lapic(LVT_TIMER_ADDR, timer_reg);

	write_lapic(INITIAL_COUNT_ADDR, TIC_DEFUALT_VALUE);

	// Disbale logical interrupt lines
	lvt_lint_reg lint0{ .masked=true }, lint1{ .masked=true };
	write_lapic(LINT0_ADDR, lint0);
	write_lapic(LINT1_ADDR, lint1);

	// Disable performance counter overflow interrupts
	// on machines that provide that interrupt entry.
	auto ver = read_lapic<lapic_version_reg>(VERSION_ADDR);
	if (ver.max_lvt >= 4u)
	{
		lvt_perf_mon_counters_reg reg{ .mask=true };
		write_lapic(LVT_PC_ADDR, reg);
	}

	// Map error interrupt to IRQ_ERROR.
	lvt_error_reg error_reg{ .vector=trap::IRQ_TO_TRAPNUM(IRQ_ERROR) };
	write_lapic(ERROR_ADDR, error_reg);

	// Clear error status register (requires back-to-back writes).
	error_status_reg esr{ .error_bits=0, .res0=0 };
	write_lapic(ESR_ADDR, esr);
	write_lapic(ESR_ADDR, esr);

	// Ack any outstanding interrupts.
	write_lapic(EOI_ADDR, 0);

	// Send an Init Level De-Assert to synchronise arbitration ID's.
	lapic_icr_reg icr{ .value_high=0, .value_low=0 };
	icr.dest_shorthand = APIC_DEST_SHORTHAND_ALL_AND_SELF;
	icr.delivery_mode = DLM_INIT;
	icr.trigger = TRG_LEVEL;

	write_lapic(ICR_HI_ADDR, icr.value_high);
	write_lapic(ICR_LO_ADDR, icr.value_low);

	while (read_lapic<uint32_t>(ICR_LO_ADDR) & 0x00001000);

	// Enable interrupts on the APIC (but not on the processor).
	lapic_task_priority_reg reg{ .p_class=0, .p_subclass=0 };
	write_lapic(TASK_PRI_ADDR, reg);
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

	if (lapic_base == nullptr)
	{
		return 0;
	}

	auto id_reg = read_lapic<lapic_id_reg>(ID_ADDR);

	for (size_t i = 0; i < cpu_count; i++)
	{
		if (id_reg.apic_id == cpus[i].apicid)
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
	auto wrv = reinterpret_cast<uint16_t*>(P2V((0x40 << 4 | 0x67))); // Warm reset vector
	wrv[0] = 0;
	wrv[1] = addr >> 4;

	// "Universal startup algorithm."
	// Send INIT (level-triggered) interrupt to reset other CPU.
	{
		lapic_icr_reg icr{ .dest=static_cast<uint32_t>(apicid),
			.delivery_mode=DLM_INIT,
			.trigger=TRG_LEVEL,
			.level=APIC_LEVEL_ASSERT };

		write_lapic(ICR_HI_ADDR, icr.value_high);
		write_lapic(ICR_LO_ADDR, icr.value_low);

		delay_a_while(200);
	}

	{
		lapic_icr_reg icr{ .delivery_mode=DLM_INIT, .trigger=TRG_LEVEL };
		write_lapic(ICR_LO_ADDR, icr.value_low);
		delay_a_while(10000);
	}

	// Send startup IPI (twice!) to enter code.
	// Regular hardware is supposed to only accept a STARTUP
	// when it is in the halted state due to an INIT.  So the second
	// should be ignored, but it is part of the official Intel algorithm.
	for (int i = 0; i < 2; i++)
	{
		lapic_icr_reg icr{ .dest=static_cast<uint32_t>(apicid),
			.delivery_mode=DLM_STARTUP,
			.vector=static_cast<uint32_t>((addr >> 12)) };

		write_lapic(ICR_HI_ADDR, icr.value_high);
		write_lapic(ICR_LO_ADDR, icr.value_low);

		delay_a_while(200);
	}
}

void local_apic::write_eoi()
{
	if (lapic_base)
	{
		eoi_reg eoi = 0;
		write_lapic(EOI_ADDR, eoi);
	}
}

void local_apic::apic_send_ipi(uint32_t dst_apic_id, delievery_modes mode, uint32_t vec, irq_destinations irq_dest)
{
	lapic_icr_reg icr{};
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

	write_lapic(ICR_HI_ADDR, icr.value_high);
	write_lapic(ICR_LO_ADDR, icr.value_low);
}

void local_apic::apic_send_ipi(uint32_t dst_apic_id, delievery_modes mode, uint32_t vec)
{
	apic_send_ipi(dst_apic_id, mode, vec, IRQDEST_SINGLE);
}

void local_apic::apic_broadcast_ipi(delievery_modes mode, uint32_t vec)
{
	apic_send_ipi(BROADCAST_DST_APIC_ID, mode, vec, IRQDST_BROADCAST);
}
