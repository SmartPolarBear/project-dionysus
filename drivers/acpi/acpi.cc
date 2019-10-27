#include "drivers/acpi/acpi.h"
#include "drivers/console/console.h"
#include "drivers/debug/kdebug.h"
#include "sys/memlayout.h"
#include "sys/mmu.h"
#include "sys/multiboot.h"

using acpi::acpi_madt;
using acpi::acpi_rsdp;
using acpi::acpi_rsdt;

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

    if(!arr_cmp(rsdp->signature,SIGNATURE_RSDP,8))
    {
        KDEBUG_GENERALPANIC("Invalid ACPI RSDP: failed to check signature.");
    }

    KDEBUG_ASSERT(rsdp != nullptr);

    console::printf("acpi rsdp is at 0x%x\n", rsdp);
}