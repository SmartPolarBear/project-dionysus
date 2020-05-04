#include "../acpi.h"
#include "../v1/acpi_v1.h"
#include "../v2/acpi_v2.h"

#include "sys/memlayout.h"
#include "sys/mmu.h"
#include "sys/multiboot.h"

#include "drivers/acpi/acpi.h"
#include "drivers/acpi/cpu.h"
#include "drivers/apic/apic.h"
#include "drivers/apic/traps.h"
#include "drivers/console/console.h"
#include "drivers/debug/kdebug.h"

#include <cstring>
// #include "lib/libcxx/new"

using acpi::acpi_desc_header;
using acpi::acpi_madt;
using acpi::acpi_rsdp;
using acpi::acpi_rsdt;
using acpi::acpi_xsdt;
using acpi::madt_entry_header;
using acpi::madt_ioapic;
using acpi::madt_iso;

using trap::TRAP_NUMBERMAX;

bool acpi_sdt_checksum(const acpi::acpi_desc_header *header)
{
    uint8_t sum = 0;
    for (size_t i = 0; i < header->length; i++)
    {
        sum += ((char *)header)[i];
    }
    return sum == 0;
}