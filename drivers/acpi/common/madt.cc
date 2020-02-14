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


madt_ioapic ioapics[CPU_COUNT_LIMIT] = {};
size_t ioapic_count = 0;

madt_iso intr_src_overrides[TRAP_NUMBERMAX] = {};
size_t iso_count = 0;


void acpi_madt_init(const acpi_madt *madt)
{
    KDEBUG_ASSERT(madt != nullptr);
    KDEBUG_ASSERT(madt->header.length >= sizeof(*madt));

    local_apic::lapic = IO2V<decltype(local_apic::lapic)>((void *)static_cast<uintptr_t>(madt->lapic_addr_phys));

    const madt_entry_header *begin = reinterpret_cast<decltype(begin)>(madt->table),
                            *end = reinterpret_cast<decltype(begin)>(madt->table + madt->header.length - sizeof(*madt));

    auto next_entry = [](auto entry) {
        return reinterpret_cast<decltype(entry)>((void *)(uintptr_t(entry) + entry->length));
    };

    for (auto entry = const_cast<madt_entry_header *>(begin);
         entry != end;
         entry = next_entry(entry))
    {
        switch (entry->type)
        {
        case acpi::MADT_ENTRY_LAPIC:
        {
            acpi::madt_lapic *lapic = reinterpret_cast<decltype(lapic)>(entry);
            KDEBUG_ASSERT(sizeof(*lapic) == lapic->length);

            if (!(lapic->flags & acpi::APIC_LAPIC_ENABLED))
            {
                break;
            }

            cpus[cpu_count] = {.id = cpu_count, .apicid = lapic->apic_id, .present = true};
            cpu_count++;
            break;
        }
        case acpi::MADT_ENTRY_IOAPIC:
        {
            acpi::madt_ioapic *ioapic = reinterpret_cast<decltype(ioapic)>(entry);
            KDEBUG_ASSERT(sizeof(*ioapic) == ioapic->length);

            ioapics[ioapic_count] = madt_ioapic{*ioapic};
            ioapic_count++;
            break;
        }
        case acpi::MADT_ENTRY_ISO:
        {
            acpi::madt_iso *iso = reinterpret_cast<decltype(iso)>(entry);
            KDEBUG_ASSERT(sizeof(*iso) == iso->length);

            intr_src_overrides[iso_count] = madt_iso{*iso};
            iso_count++;
            break;
        }
        default:
            break;
        }
    }

    // the kernel must run with at lease 2 CPUs
    KDEBUG_ASSERT(cpu_count >= 1);
}


size_t acpi::get_ioapic_count(void)
{
    return ioapic_count;
}

void acpi::get_ioapics(madt_ioapic res[], size_t bufsz)
{
    for (size_t i = 0; i < bufsz; i++)
    {
        res[i] = madt_ioapic{ioapics[i]};
    }
}

madt_ioapic acpi::get_first_ioapic(void)
{
    return ioapics[0];
}

size_t acpi::get_iso_count(void)
{
    return iso_count;
}

void acpi::get_isos(madt_iso res[], size_t bufsz)
{
    for (size_t i = 0; i < bufsz; i++)
    {
        res[i] = madt_iso{intr_src_overrides[i]};
    }
}