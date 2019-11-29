/**
 * @ Author: SmartPolarBear
 * @ Create Time: 1970-01-01 08:00:00
 * @ Modified by: SmartPolarBear
 * @ Modified time: 2019-11-29 23:13:56
 * @ Description:
 */

#include "sys/memlayout.h"
#include "sys/mmu.h"
#include "sys/multiboot.h"

#include "drivers/acpi/acpi.h"
#include "drivers/apic/apic.h"
#include "drivers/console/console.h"
#include "drivers/debug/kdebug.h"

#include "lib/libc/string.h"
#include "lib/libcxx/new"

// declared in acpi.h
struct cpu cpus[CPU_COUNT_LIMIT] = {};
size_t cpu_max_idx = 0;

size_t ioapic_ids[CPU_COUNT_LIMIT] = {};
size_t ioapic_max_idx = 0;

using acpi::acpi_desc_header;
using acpi::acpi_madt;
using acpi::acpi_rsdp;
using acpi::acpi_rsdt;
using acpi::acpi_xsdt;
using acpi::madt_entry_header;

static inline bool acpi_sdt_checksum(acpi::acpi_desc_header *header)
{
    uint8_t sum = 0;
    for (size_t i = 0; i < header->length; i++)
    {
        sum += ((char *)header)[i];
    }
    return sum == 0;
}

static void acpi_init_smp(const acpi_madt *madt)
{
    KDEBUG_ASSERT(madt != nullptr);
    KDEBUG_ASSERT(madt->header.length >= sizeof(*madt));

    apic::lapic = IO2V<decltype(apic::lapic)>((void *)madt->lapic_addr_phys);

    const madt_entry_header *begin = reinterpret_cast<decltype(begin)>(madt->table),
                            *end = reinterpret_cast<decltype(begin)>(madt->table + madt->header.length - sizeof(*madt));

    auto next_entry = [](auto entry) {
        return reinterpret_cast<decltype(entry)>((void *)(uintptr_t(entry) + entry->length));
    };

    for (auto entry = const_cast<madt_entry_header *>(begin);
         entry < end;
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

            console::printf("ACPI: CPU#%d ACPI ID %d\n", cpu_max_idx, lapic->apic_id);

            cpus[cpu_max_idx].id = cpu_max_idx;
            cpus[cpu_max_idx].apicid = lapic->apic_id;
            ++cpu_max_idx;
            break;
        }
        case acpi::MADT_ENTRY_IOAPIC:
        {
            acpi::madt_ioapic *ioapic = reinterpret_cast<decltype(ioapic)>(entry);
            KDEBUG_ASSERT(sizeof(*ioapic) == ioapic->length);

            console::printf("ACPI: IOAPIC#%d @ 0x%x ID=%d, BASE=%d\n",
                            ioapic_max_idx, ioapic->addr, ioapic->id, ioapic->interrupt_base);

            ioapic_ids[ioapic_max_idx] = ioapic->id;
            ioapic_max_idx++;
            break;
        }
        default:
            break;
        }
    }

    // the kernel must run with at lease 2 CPUs
    KDEBUG_ASSERT(cpu_max_idx >= 1);
}

static void acpi_init_v1(const acpi_rsdp *rsdp)
{
    KDEBUG_ASSERT(rsdp->xsdt_addr_phys < PHYMEMORY_SIZE);

    acpi_rsdt *rsdt = reinterpret_cast<acpi_rsdt *>(P2V(rsdp->rsdt_addr_phys));
    size_t entrycnt = (rsdt->header.length - sizeof(rsdt->header)) / 4;

    KDEBUG_ASSERT(acpi_sdt_checksum(&rsdt->header) == true);

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

    KDEBUG_ASSERT(madt != nullptr);

    acpi_init_smp(madt);
}

static void acpi_init_v2(const acpi_rsdp *rsdp)
{
    KDEBUG_ASSERT(rsdp->xsdt_addr_phys < PHYMEMORY_SIZE);

    acpi_xsdt *xsdt = reinterpret_cast<acpi_xsdt *>(P2V(rsdp->xsdt_addr_phys));
    size_t entrycnt = (xsdt->header.length - sizeof(xsdt->header)) / 8;

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

    acpi_init_smp(madt);
}

void acpi::acpi_init(void)
{
    auto acpi_new_tag = multiboot::aquire_tag_ptr<multiboot_tag_new_acpi>(MULTIBOOT_TAG_TYPE_ACPI_NEW);
    auto acpi_old_tag = multiboot::aquire_tag_ptr<multiboot_tag_new_acpi>(MULTIBOOT_TAG_TYPE_ACPI_OLD);

    if (acpi_new_tag == nullptr && acpi_old_tag == nullptr)
    {
        KDEBUG_GENERALPANIC("ACPI is not compatible with the machine.");
    }

    acpi_rsdp *rsdp = (acpi_new_tag != nullptr)
                          ? rsdp = reinterpret_cast<decltype(rsdp)>(acpi_new_tag->rsdp)
                          : rsdp = reinterpret_cast<decltype(rsdp)>(acpi_old_tag->rsdp);

    if (strncmp((char *)rsdp->signature, SIGNATURE_RSDP, strlen(SIGNATURE_RSDP)) != 0)
    {
        KDEBUG_GENERALPANIC("Invalid ACPI RSDP: failed to check signature.");
    }

    KDEBUG_ASSERT(rsdp != nullptr);

    if (rsdp->revision == RSDP_REV1)
    {
        acpi_init_v1(rsdp);
    }
    else
    {
        acpi_init_v2(rsdp);
    }
}