/**
 * @ Author: SmartPolarBear
 * @ Create Time: 1970-01-01 08:00:00
 * @ Modified by: SmartPolarBear
 * @ Modified time: 2019-11-24 17:42:35
 * @ Description:
 */

#include "sys/memlayout.h"
#include "sys/mmu.h"
#include "sys/multiboot.h"

#include "drivers/acpi/acpi.h"
#include "drivers/console/console.h"
#include "drivers/debug/kdebug.h"

#include "lib/libcxx/new"

// declared in acpi.h
struct cpu cpus[CPU_COUNT_LIMIT] = {};
size_t cpu_count = 0;

using acpi::acpi_desc_header;
using acpi::acpi_madt;
using acpi::acpi_rsdp;
using acpi::acpi_rsdt;
using acpi::acpi_xsdt;

template <typename TA, typename TB>
static inline size_t arr_cmp(TA *ita, TB *itb, size_t len)
{
    for (size_t i = 0; i < len; i++)
    {
        if (*(ita + i) != *(itb + i))
        {
            return i;
        }
    }

    return 0;
}

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
    KDEBUG_ASSERT(madt->header.length >= sizeof(decltype(*madt)));

    uintptr_t lapic_addr = madt->lapic_addr_phys;

    for (uint8_t *p = (uint8_t *)madt->table,
                 *e = p + madt->header.length - sizeof(decltype(*madt));
         p < e;)
    {
        size_t len = 0;
        if (e - p < 2)
        {
            break;
        }

        len = p[1];

        if (e - p < len)
        {
            break;
        }

        switch (p[0])
        {
        case acpi::ENTRY_TYPE_LAPIC:
        {
            acpi::madt_lapic *lapic = reinterpret_cast<decltype(lapic)>(p);
            if (len < sizeof(*lapic))
            {
                break;
            }
            if (!(lapic->flags & acpi::APIC_LAPIC_ENABLED))
            {
                break;
            }

            console::printf("ACPI: CPU#%d ACPI ID %d\n", cpu_count, lapic->apic_id);

            cpus[cpu_count].id = cpu_count;
            cpus[cpu_count].apicid = lapic->apic_id;
            ++cpu_count;
            break;
        }
        case acpi::ENTRY_TYPE_IOAPIC:
        {
            acpi::madt_ioapic *ioapic = reinterpret_cast<decltype(ioapic)>(p);
            if (len < sizeof(*ioapic))
            {
                break;
            }
            console::printf("ACPI: IOAPIC#%d@0x%x ID=%d, BASE=%d\n",
                            ioapic_count, ioapic->addr, ioapic->id, ioapic->interrupt_base);
            
            break;
        }
        default:
            break;
        }

        p += len;
    }
}

static void acpi_init_v1(const acpi_rsdp *rsdp)
{
    KDEBUG_ASSERT(rsdp->xsdt_addr_phys < PHYMEMORY_SIZE);

    acpi_rsdt *rsdt = reinterpret_cast<acpi_rsdt *>(P2V(rsdp->rsdt_addr_phys));
    size_t entrycnt = (rsdt->header.length - sizeof(*rsdt)) / 4;

    KDEBUG_ASSERT(acpi_sdt_checksum(&rsdt->header) == true);

    acpi_madt *madt = nullptr;
    for (size_t i = 0; i < entrycnt; i++)
    {
        auto header = reinterpret_cast<acpi_desc_header *>(P2V(rsdt->entry[i]));
        if (arr_cmp(header->signature, acpi::SIGNATURE_MADT, 4) == 0)
        {
            madt = reinterpret_cast<decltype(madt)>(header);
        }
    }

    KDEBUG_ASSERT(madt != nullptr);

    acpi_init_smp(madt);
}

static void acpi_init_v2(const acpi_rsdp *rsdp)
{
    KDEBUG_ASSERT(rsdp->xsdt_addr_phys < PHYMEMORY_SIZE);

    acpi_xsdt *xsdt = reinterpret_cast<acpi_xsdt *>(P2V(rsdp->xsdt_addr_phys));
    size_t entrycnt = (xsdt->header.length - sizeof(*xsdt)) / 8;

    KDEBUG_ASSERT(acpi_sdt_checksum(&xsdt->header) == true);

    console::printf("Init acpi v2 entries %d\n", entrycnt);
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

    if (arr_cmp(rsdp->signature, SIGNATURE_RSDP, 8) != 0)
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