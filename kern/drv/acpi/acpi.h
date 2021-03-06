#pragma once

#include "drivers/acpi/acpi.h"

#include "system/error.hpp"

#include "boot/multiboot2.h"

struct acpi_version_adapter
{
	multiboot_tag_new_acpi* acpi_tag;

	error_code (* init_sdt)(const acpi::acpi_rsdp* rsdp);
};

extern acpi_version_adapter acpiv1_adapter;
extern acpi_version_adapter acpiv2_adapter;

// madt.cc
[[nodiscard]] error_code acpi_madt_init(const acpi::acpi_madt* madt);

[[nodiscard]] error_code acpi_mcfg_init(const acpi::acpi_mcfg* mcfg);

[[nodiscard]] error_code acpi_fadt_init(const acpi::acpi_fadt* _fadt);