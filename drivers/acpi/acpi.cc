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
            console::printf("i=%d\n", i);
            return false;
        }
    }

    return true;
}

void acpi::acpi_init(void)
{
    acpi_rsdp *rsdp = nullptr;
    acpi_madt *madt = nullptr;
    acpi_rsdt *rsdt = nullptr;

    auto acpi_new_tag = reinterpret_cast<multiboot_tag_new_acpi *>(multiboot::aquire_tag(MULTIBOOT_TAG_TYPE_ACPI_NEW));
    auto acpi_old_tag = reinterpret_cast<multiboot_tag_old_acpi *>(multiboot::aquire_tag(MULTIBOOT_TAG_TYPE_ACPI_OLD));

    console::printf("acpi_new_tag is at 0x%x,acpi_old_tag is at 0x%x\n", (uintptr_t)acpi_new_tag, (uintptr_t)acpi_old_tag);
    rsdp = reinterpret_cast<decltype(rsdp)>(acpi_old_tag->rsdp);

    return;
    if (acpi_new_tag != nullptr && (rsdp = reinterpret_cast<decltype(rsdp)>(acpi_new_tag->rsdp), arr_cmp(rsdp->signature, SIGNATURE_RSDP, 8)))
    {
    }
    else if (acpi_old_tag != nullptr && (rsdp = reinterpret_cast<decltype(rsdp)>(acpi_old_tag->rsdp), arr_cmp(rsdp->signature, SIGNATURE_RSDP, 8)))
    {
    }
    else
    {
        KDEBUG_GENERALPANIC("Both new ACPI and old ACPI can't be obtain.\nThe hardware is obsoluted.");
    }

    KDEBUG_ASSERT(rsdp != nullptr);

    console::printf("acpi rsdp is at 0x%x\n", rsdp);
}