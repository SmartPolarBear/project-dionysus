#include "arch/amd64/port_io.h"

#include "system/types.h"

#include "drivers/pci/pci.hpp"
#include "drivers/acpi/acpi.h"

// initialize PCI and PCIe
void pci::pci_init()
{
	auto mcfg = acpi::get_mcfg();

	auto ret = express::pcie_init(mcfg);
	if (ret != ERROR_SUCCESS)
	{
		KDEBUG_RICHPANIC_CODE(ret, true, "PCIE initialization failed");
	}
}