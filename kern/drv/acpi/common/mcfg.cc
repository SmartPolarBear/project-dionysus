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
#include "debug/kdebug.h"

#include <cstring>
#include <algorithm>

using namespace acpi;

acpi_mcfg* mcfg = nullptr;

[[nodiscard]] error_code acpi_mcfg_init(const acpi::acpi_mcfg* _mcfg)
{
	mcfg = const_cast<decltype(mcfg)>(_mcfg);

	if (!acpi_header_valid(&mcfg->header))
	{
		return -ERROR_INVALID;
	}

	return ERROR_SUCCESS;
}

acpi_mcfg* acpi::get_mcfg()
{
	return mcfg;
}