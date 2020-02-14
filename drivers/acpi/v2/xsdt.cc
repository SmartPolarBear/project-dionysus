#include "../acpi.h"
#include "acpi_v2.h"

#include "sys/memlayout.h"
#include "sys/mmu.h"
#include "sys/multiboot.h"

#include "drivers/acpi/acpi.h"
#include "drivers/acpi/cpu.h"
#include "drivers/apic/apic.h"
#include "drivers/apic/traps.h"
#include "drivers/console/console.h"
#include "drivers/debug/kdebug.h"

#include "lib/libc/string.h"
#include "lib/libcxx/new"

using acpi::acpi_desc_header;
using acpi::acpi_madt;
using acpi::acpi_rsdp;
using acpi::acpi_rsdt;
using acpi::acpi_xsdt;
using acpi::madt_entry_header;
using acpi::madt_ioapic;
using acpi::madt_iso;

using trap::TRAP_NUMBERMAX;

hresult init_xsdt(const acpi::acpi_rsdp *rsdp)
{
    acpi_xsdt *xsdt = reinterpret_cast<acpi_xsdt *>(P2V(rsdp->xsdt_addr_phys));

    size_t entrycnt = (xsdt->header.length - sizeof(xsdt->header)) / 4;

    KDEBUG_ASSERT(acpi_sdt_checksum(&xsdt->header) == true);

    acpi_madt *madt = nullptr;
    for (size_t i = 0; i < entrycnt; i++)
    {
        auto header = reinterpret_cast<acpi_desc_header *>(P2V(xsdt->entry[i]));
        if (strncmp((char *)header->signature, acpi::SIGNATURE_MADT, strlen(acpi::SIGNATURE_MADT)) == 0)
        {
            madt = reinterpret_cast<decltype(madt)>(header);
            break;
        }
    }

    KDEBUG_ASSERT(madt != nullptr);

    acpi_madt_init(madt);

    return ERROR_SUCCESS;
}