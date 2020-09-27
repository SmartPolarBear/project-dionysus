#include "../acpi.h"
#include "../v1/acpi_v1.h"
#include "../v2/acpi_v2.h"

#include "system/memlayout.h"
#include "system/mmu.h"
#include "system/multiboot.h"

#include "drivers/acpi/acpi.h"
#include "drivers/acpi/cpu.h"
#include "drivers/apic/apic.h"
#include "drivers/apic/traps.h"
#include "drivers/console/console.h"
#include "drivers/debug/kdebug.h"

#include <cstring>
#include <algorithm>

using acpi::acpi_desc_header;
using acpi::acpi_madt;
using acpi::acpi_rsdp;
using acpi::acpi_rsdt;
using acpi::acpi_xsdt;
using acpi::madt_entry_header;
using acpi::madt_ioapic;
using acpi::madt_iso;
using acpi::acpi_fadt;

using trap::TRAP_NUMBERMAX;

using std::min;
using std::max;

acpi_fadt* fadt = nullptr;

[[nodiscard]] error_code acpi_fadt_init(const acpi::acpi_fadt* _fadt)
{
	fadt = const_cast<decltype(fadt)>(_fadt);

	if (!acpi_header_valid(&fadt->header))
	{
		return -ERROR_INVALID;
	}

	return ERROR_SUCCESS;
}

acpi_fadt* acpi::get_fadt()
{
	return fadt;
}
