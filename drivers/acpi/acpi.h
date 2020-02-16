#if !defined(__DRIVERS_ACPI_ACPI_H)
#define __DRIVERS_ACPI_ACPI_H

#include "drivers/acpi/acpi.h"

#include "sys/error.h"

#include "boot/multiboot2.h"

struct acpi_version_adapter
{
    multiboot_tag_new_acpi *acpi_tag;

    error_code (*init_sdt)(const acpi::acpi_rsdp *rsdp);
};

extern acpi_version_adapter acpiv1_adapter;
extern acpi_version_adapter acpiv2_adapter;

// sdt.cc
bool acpi_sdt_checksum(const acpi::acpi_desc_header *header);

// madt.cc
[[nodiscard]] error_code acpi_madt_init(const acpi::acpi_madt *madt);

#endif // __DRIVERS_ACPI_ACPI_H
