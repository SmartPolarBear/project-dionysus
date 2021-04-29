#include "arch/amd64/cpu/x86.h"

#include "drivers/acpi/acpi.h"
#include "drivers/acpi/cpu.h"
#include "drivers/apic/apic.h"
#include "drivers/apic/traps.h"
#include "drivers/console/console.h"
#include "debug/kdebug.h"

#include "system/memlayout.h"
#include "system/mmu.h"

#include "builtin_text_io.hpp"

using namespace apic;

using trap::TRAP_IRQ0;

using namespace local_apic;
using namespace io_apic;

/* Memory Mapped Registers For Accessing IOAPIC Registers :
 * Memory Address Mnemonic   Register Name               Access    D/I# SIgnal
 *  FEC0  xy00h   IOREGSEL   I/O Register Select (index) R/W       0
 *  FEC0  xy10h   IOWIN      I/O Window (data)           R/W       1
 */
void apic::io_apic::write_ioapic(uintptr_t apic_base, uint8_t reg, uint32_t val)
{
	// tell IOREGSEL where we want to write to
	*(volatile uint32_t*)(apic_base) = reg;
	// write the value to IOWIN
	*(volatile uint32_t*)(apic_base + 0x10) = val;
}

uint32_t apic::io_apic::read_ioapic(uintptr_t apic_base, uint8_t reg)
{
	// tell IOREGSEL where we want to read from
	*(volatile uint32_t*)(apic_base) = reg;
	// return the kbl from IOWIN
	return *(volatile uint32_t*)(apic_base + 0x10);
}

PANIC void io_apic::init_ioapic()
{
	pic8259A::init_pic();

	auto ioapic = acpi::first_ioapic();

	uintptr_t ioapic_addr = IO2V(ioapic->addr);

	write_ioapic(ioapic_addr, IOAPICID, ioapic->id);

	auto id_reg = read_ioapic<io_apic_id_reg>(ioapic_addr, IOAPICID);
	auto ver_reg = read_ioapic<io_apic_version_reg>(ioapic_addr, IOAPICVER);

	if (id_reg.id != ioapic->id)
	{
		write_format("WARNING: inconsistency between apic id from IOAPICID register (%d) and ioapic.id (%d)\n",
			id_reg.id,
			ioapic->id);
	}

	for (size_t i = 0; i <= ver_reg.max_redir_entries; i++)
	{
		io_apic_redtbl_reg redtbl{};

		redtbl.vector = TRAP_IRQ0 + i;
		redtbl.delievery_mode = local_apic::DLM_FIXED;
		redtbl.destination_mode = DTM_PHYSICAL;
		redtbl.polarity = 0;
		redtbl.trigger_mode = TRG_EDGE;
		redtbl.mask = true; // set to true to disable it
		redtbl.destination_id = 0;

		auto[low, high]=IOAPIC_REDTBL_ENTRY_REG(i);

		write_ioapic(ioapic_addr, low, redtbl.raw_low);
		write_ioapic(ioapic_addr, high, redtbl.raw_high);
	}
};

void io_apic::write_redtbl(uint32_t trapnum, uint32_t cpu_acpi_id_rounted)
{
	io_apic_redtbl_reg redir_new{};

	redir_new.vector = TRAP_IRQ0 + trapnum;
	redir_new.delievery_mode = DLM_FIXED;
	redir_new.destination_mode = DTM_PHYSICAL;
	redir_new.polarity = 0;
	redir_new.trigger_mode = TRG_EDGE;
	redir_new.mask = false;
	redir_new.destination_id = cpu_acpi_id_rounted;

	auto ioapic = acpi::first_ioapic();

	uintptr_t ioapic_addr = IO2V(ioapic->addr);

	auto[low, high]=IOAPIC_REDTBL_ENTRY_REG(trapnum);

	write_ioapic(ioapic_addr, low, redir_new.raw_low);
	write_ioapic(ioapic_addr, high, redir_new.raw_high);
}
