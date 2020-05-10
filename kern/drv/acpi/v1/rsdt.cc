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

using acpi::acpi_desc_header;
using acpi::acpi_madt;
using acpi::acpi_rsdp;
using acpi::acpi_rsdt;
using acpi::acpi_xsdt;
using acpi::madt_entry_header;
using acpi::madt_ioapic;
using acpi::madt_iso;

using trap::TRAP_NUMBERMAX;

error_code init_rsdt(const acpi::acpi_rsdp *rsdp)
{
    acpi_rsdt *rsdt = reinterpret_cast<acpi_rsdt *>(P2V(rsdp->rsdt_addr_phys));

    size_t entrycnt = (rsdt->header.length - sizeof(rsdt->header)) / 4;

    // KDEBUG_ASSERT(acpi_sdt_checksum(&rsdt->header) == true);
    if (!acpi_sdt_checksum(&rsdt->header))
    {
        return -ERROR_INVALID_DATA;
    }

    acpi_madt *madt = nullptr;
    for (size_t i = 0; i < entrycnt; i++)
    {
        auto header = reinterpret_cast<acpi_desc_header *>(P2V(rsdt->entry[i]));
        if (strncmp((char *)header->signature, acpi::SIGNATURE_MADT, strlen(acpi::SIGNATURE_MADT)) == 0)
        {

            madt = reinterpret_cast<decltype(madt)>(header);
            break;
        }
    }


    return acpi_madt_init(madt);
}