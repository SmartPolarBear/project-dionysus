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
    KDEBUG_ASSERT(rsdp->xsdt_addr_phys < V2P(KERNEL_VIRTUALBASE));
    
    acpi_rsdt *rsdt = reinterpret_cast<acpi_rsdt *>(rsdp->rsdt_addr_phys);
    int k = sizeof(acpi_rsdp);
    size_t entrycnt = (rsdt->header.length - sizeof(*rsdt)) / 4;

    // KDEBUG_ASSERT(acpi_sdt_checksum(&rsdt->header) == true);

    console::printf("Init acpi v1 entries %d\n", entrycnt);
}

static void acpi_init_v2(const acpi_rsdp *rsdp)
{
    KDEBUG_ASSERT(rsdp->xsdt_addr_phys < V2P(KERNEL_VIRTUALBASE));

    acpi_xsdt *xsdt = reinterpret_cast<acpi_xsdt *>(P2V(rsdp->xsdt_addr_phys));
    size_t entrycnt = (xsdt->header.length - sizeof(*xsdt)) / 8;

    // KDEBUG_ASSERT(acpi_sdt_checksum(&xsdt->header) == true);
    console::printf("Init acpi v2 entries %d\n", entrycnt);
}

static acpi_rsdp *scan_rsdp(const char *start, const char *end)
{
    // for (char *p = p2v(base); len >= sizeof(struct acpi_rdsp); len -= 4, p += 4)
    // {
    //     if (memcmp(p, SIG_RDSP, 8) == 0)
    //     {
    //         uint sum, n;
    //         for (sum = 0, n = 0; n < 20; n++)
    //             sum += p[n];
    //         if ((sum & 0xff) == 0)
    //             return (struct acpi_rdsp *)p;
    //     }
    // }
    // return (struct acpi_rdsp *)0;

    for (char *p = const_cast<char *>(start); p != end; p++)
    {
        if (arr_cmp(p, acpi::SIGNATURE_RSDP, 8))
        {
            size_t checksum = 0;
            for (size_t i = 0; i < 20; i++)
            {
                checksum += p[i];
            }

            if ((checksum & 0XFF) == 0)
            {
                return reinterpret_cast<acpi_rsdp *>(p);
            }
        }
    }
    return nullptr;
}

static acpi_rsdp *find_rsdp(void)
{
    const char *ebda_phy = new ((void *)0x40E) char; //extended BIOS data area
    const char *mainbios_phy = new ((void *)0x000E0000) char;

    constexpr size_t ebda_size = 1024;
    constexpr size_t mainbios_size = 0x000FFFFF - 0x000E0000;

    auto rsdp = scan_rsdp(P2V_WO(ebda_phy), P2V_WO(ebda_phy + ebda_size));
    rsdp = rsdp != nullptr ? rsdp : scan_rsdp(P2V_WO(mainbios_phy), P2V_WO(mainbios_phy + mainbios_size));

    KDEBUG_ASSERT(rsdp != nullptr);

    return rsdp;
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

    console::printf("acpi=%d\n", rsdp->rsdt_addr_phys);

    return;
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