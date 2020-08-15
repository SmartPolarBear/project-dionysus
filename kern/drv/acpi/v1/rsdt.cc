#include "../acpi.h"
#include "acpi_v1.h"

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

using namespace acpi;

using trap::TRAP_NUMBERMAX;

error_code init_rsdt(const acpi::acpi_rsdp* rsdp)
{
	acpi_rsdt* rsdt = reinterpret_cast<acpi_rsdt*>(P2V(rsdp->rsdt_addr_phys));

	if (!acpi_header_valid(&rsdt->header))
	{
		return -ERROR_INVALID;
	}

	acpi_madt* madt = nullptr;
	acpi_mcfg* mcfg = nullptr;
	for (size_t i = 0;
		 i < (rsdt->header.length - sizeof(acpi_desc_header)) / sizeof(uint32_t);
		 i++)
	{
		auto header = reinterpret_cast<acpi_desc_header*>(P2V(rsdt->entry[i]));
		if (strncmp((char*)header->signature, acpi::SIGNATURE_MADT, strlen(acpi::SIGNATURE_MADT)) == 0)
		{
			madt = reinterpret_cast<decltype(madt)>(header);
		}
		else if (strncmp((char*)header->signature, acpi::SIGNATURE_MCFG, strlen(acpi::SIGNATURE_MCFG)) == 0)
		{
			mcfg = reinterpret_cast<decltype(mcfg)>(header);
		}
	}

	auto ret = acpi_madt_init(madt);
	if (ret != ERROR_SUCCESS)
	{
		return ret;
	}

	ret = acpi_mcfg_init(mcfg);

	return ret;
}