/**
 * @ Author: SmartPolarBear
 * @ Create Time: 1970-01-01 08:00:00
 * @ Modified by: SmartPolarBear
 * @ Modified time: 2019-11-23 13:55:55
 * @ Description:
 */

#include "drivers/acpi/acpi.h"
#include "drivers/console/console.h"
#include "drivers/debug/kdebug.h"
#include "sys/memlayout.h"
#include "sys/mmu.h"
#include "sys/multiboot.h"

#include "lib/libcxx/new"

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
            return false;
        }
    }

    return true;
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

static void acpi_init_v1(const acpi_rsdp *rsdp)
{
    KDEBUG_ASSERT(rsdp->xsdt_addr_phys < PHYMEMORY_SIZE);

    acpi_rsdt *rsdt = reinterpret_cast<acpi_rsdt *>(P2V(rsdp->rsdt_addr_phys));
    size_t entrycnt = (rsdt->header.length - sizeof(*rsdt)) / 4;

    KDEBUG_ASSERT(acpi_sdt_checksum(&rsdt->header) == true);

    console::printf("Init acpi v1 entries %d\n", entrycnt);
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

    if (!arr_cmp(rsdp->signature, SIGNATURE_RSDP, 8))
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