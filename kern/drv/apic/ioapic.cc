#include "arch/amd64/cpu/x86.h"

#include "drivers/acpi/acpi.h"
#include "drivers/acpi/cpu.h"
#include "drivers/apic/apic.h"
#include "drivers/apic/traps.h"
#include "drivers/console/console.h"
#include "debug/kdebug.h"

#include "system/memlayout.h"
#include "system/mmu.h"

#include "../../libs/basic_io/include/builtin_text_io.hpp"

using namespace apic;

using trap::TRAP_IRQ0;

enum ioapic_regs
{
	IOAPICID = 0x00,
	IOAPICVER = 0x01,
	IOAPICARB = 0x02,
	IOREDTBL_BASE = 0x10,
};

[[clang::optnone]] static inline void write_ioapic(const uintptr_t apic_base, const uint8_t reg, const uint32_t val)
{
	// tell IOREGSEL where we want to write to
	*(volatile uint32_t*)(apic_base) = reg;
	// write the value to IOWIN
	*(volatile uint32_t*)(apic_base + 0x10) = val;
}

[[clang::optnone]] static inline uint32_t read_ioapic(const uintptr_t apic_base, const uint8_t reg)
{
	// tell IOREGSEL where we want to read from
	*(volatile uint32_t*)(apic_base) = reg;
	// return the kbl from IOWIN
	return *(volatile uint32_t*)(apic_base + 0x10);
}

PANIC void io_apic::init_ioapic(void)
{
	pic8259A::initialize_pic();

	auto ioapic = acpi::first_ioapic();

	uintptr_t ioapic_addr = IO2V(ioapic->addr);

	write_ioapic(ioapic_addr, IOAPICID, ioapic->id);

	size_t apicid = (read_ioapic(ioapic_addr, IOAPICID) >> 24) & 0b1111;
	size_t redirection_count = (read_ioapic(ioapic_addr, IOAPICVER) >> 16) & 0b11111111;

	if (apicid != ioapic->id)
	{
		write_format("WARNING: inconsistence between apicid from IOAPICID register (%d) and ioapic.id (%d)\n",
			apicid,
			ioapic->id);
	}

	for (size_t i = 0; i <= redirection_count; i++)
	{
		redirection_entry redir{};

		redir.vector = TRAP_IRQ0 + i;
		redir.delievery_mode = DLM_FIXED;
		redir.destination_mode = DTM_PHYSICAL;
		redir.polarity = 0;
		redir.trigger_mode = TRG_EDGE;
		redir.mask = true; // set to true to disable it
		redir.destination_id = 0;

		write_ioapic(ioapic_addr, IOREDTBL_BASE + i * 2 + 0, redir.raw_low);
		write_ioapic(ioapic_addr, IOREDTBL_BASE + i * 2 + 1, redir.raw_high);
	}
};

void io_apic::enable_trap(uint32_t trapnum, uint32_t cpu_acpi_id_rounted)
{
	redirection_entry redir_new{};

	redir_new.vector = TRAP_IRQ0 + trapnum;
	redir_new.delievery_mode = DLM_FIXED;
	redir_new.destination_mode = DTM_PHYSICAL;
	redir_new.polarity = 0;
	redir_new.trigger_mode = TRG_EDGE;
	redir_new.mask = false;
	redir_new.destination_id = cpu_acpi_id_rounted;

	auto ioapic = acpi::first_ioapic();

	uintptr_t ioapic_addr = IO2V(ioapic->addr);

	write_ioapic(ioapic_addr, IOREDTBL_BASE + trapnum * 2 + 0, redir_new.raw_low);
	write_ioapic(ioapic_addr, IOREDTBL_BASE + trapnum * 2 + 1, redir_new.raw_high);
}
