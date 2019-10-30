#include "drivers/acpi/acpi.h"
#include "drivers/console/console.h"
#include "drivers/debug/kdebug.h"
#include "sys/memlayout.h"
#include "sys/mmu.h"
#include "sys/multiboot.h"

#include "lib/libcxx/new.h"

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
            console::printf("i=%d(*ita=%d,*itb=%d)\n", i, (int)*ita, (int)*itb);
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
    KDEBUG_ASSERT(rsdp->rsdt_addr_phys < KERNEL_PHYLIMIT);
    acpi_rsdt *rsdt = reinterpret_cast<acpi_rsdt *>(rsdp->rsdt_addr_phys);
    int k = sizeof(acpi_rsdp);
    size_t entrycnt = (rsdt->header.length - sizeof(*rsdt)) / 4;

    // KDEBUG_ASSERT(acpi_sdt_checksum(&rsdt->header) == true);

    console::printf("Init acpi v1 entries %d\n", entrycnt);
}

static void acpi_init_v2(const acpi_rsdp *rsdp)
{
    KDEBUG_ASSERT(rsdp->xsdt_addr_phys < KERNEL_PHYLIMIT);

    acpi_xsdt *xsdt = reinterpret_cast<acpi_xsdt *>(P2V(rsdp->xsdt_addr_phys));
    size_t entrycnt = (xsdt->header.length - sizeof(*xsdt)) / 8;

    // KDEBUG_ASSERT(acpi_sdt_checksum(&xsdt->header) == true);
    console::printf("Init acpi v2 entries %d\n", entrycnt);
}

static acpi_rsdp *scan_rsdp(const char *start, const char *end)
{
}

static acpi_rsdp *find_rsdp(void)
{
    const char *ebda_phy = new ((void *)0x40E) char; //extended BIOS data area
    const char *mainbios_phy = new ((void *)0x000E0000) char;

    constexpr size_t ebda_size = 1024;
    constexpr size_t mainbios_size = 0x000FFFFF - 0x000E0000;

    auto rdsp1 = scan_rsdp(P2V_WO(ebda_phy), P2V_WO(ebda_phy + ebda_size));
}

void acpi::acpi_init(void)
{
    auto acpi_new_tag = reinterpret_cast<multiboot_tag_new_acpi *>(multiboot::aquire_tag(MULTIBOOT_TAG_TYPE_ACPI_NEW));
    auto acpi_old_tag = reinterpret_cast<multiboot_tag_old_acpi *>(multiboot::aquire_tag(MULTIBOOT_TAG_TYPE_ACPI_OLD));

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