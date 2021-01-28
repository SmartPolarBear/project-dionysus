/*
 * Last Modified: Mon May 04 2020
 * Modified By: SmartPolarBear
 * -----
 * Copyright (C) 2006 by SmartPolarBear <clevercoolbear@outlook.com>
 * 
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 * -----
 * HISTORY:
 * Date      	By	Comments
 * ----------	---	----------------------------------------------------------
 */

#include "../acpi.h"
#include "../v1/acpi_v1.h"
#include "../v2/acpi_v2.h"

#include "system/memlayout.h"
#include "system/mmu.h"
#include "system/multiboot.h"

#include "drivers/acpi/acpi.h"
#include "drivers/acpi/cpu.h"
#include "drivers/apic/apic.h"
#include "drivers/apic/traps.h"
#include "drivers/console/console.h"
#include "debug/kdebug.h"

#include "ktl/span.hpp"

#include <cstring>

acpi_version_adapter* acpi_ver_adapter = nullptr;

// declared in cpu.h
cpu_struct cpus[CPU_COUNT_LIMIT] = {};
// the numbers of cpu (cores) should be within the range of uint8_t
uint8_t cpu_count = 0;

ktl::span<cpu_struct> valid_cpus{};

using acpi::acpi_desc_header;
using acpi::acpi_madt;
using acpi::acpi_rsdp;
using acpi::acpi_rsdt;
using acpi::acpi_xsdt;
using acpi::madt_entry_header;
using acpi::madt_ioapic;
using acpi::madt_iso;

using trap::TRAP_NUMBERMAX;

PANIC void acpi::init_acpi(void)
{

	auto acpi_new_tag = multiboot::acquire_tag_ptr<multiboot_tag_new_acpi>(MULTIBOOT_TAG_TYPE_ACPI_NEW);
	auto acpi_old_tag = multiboot::acquire_tag_ptr<multiboot_tag_old_acpi>(MULTIBOOT_TAG_TYPE_ACPI_OLD);

	if (acpi_new_tag != nullptr)
	{
		acpi_ver_adapter = &acpiv2_adapter;
		acpi_ver_adapter->acpi_tag = reinterpret_cast<decltype(acpi_ver_adapter->acpi_tag)>(acpi_new_tag);
	}
	else if (acpi_old_tag != nullptr)
	{
		acpi_ver_adapter = &acpiv1_adapter;
		acpi_ver_adapter->acpi_tag = reinterpret_cast<decltype(acpi_ver_adapter->acpi_tag)>(acpi_old_tag);
	}
	else
	{
		KDEBUG_RICHPANIC("ACPI is not compatible with the machine.",
			"KERNEL PANIC:ACPI",
			false, "");
	}

	acpi_rsdp* rsdp = reinterpret_cast<decltype(rsdp)>(acpi_ver_adapter->acpi_tag->rsdp);

	if (strncmp((char*)rsdp->signature, SIGNATURE_RSDP, strlen(SIGNATURE_RSDP)) != 0)
	{
		KDEBUG_RICHPANIC("Invalid ACPI RSDP: failed to check signature.",
			"KERNEL PANIC:ACPI",
			false, "The given signature is %s\n", rsdp->signature);
	}

	KDEBUG_ASSERT(rsdp != nullptr);

	auto ret = acpi_ver_adapter->init_sdt(rsdp);

	if (ret != ERROR_SUCCESS)
	{
		KDEBUG_RICHPANIC_CODE(ret, true, "");
	}

	valid_cpus = ktl::span{ cpus, cpu_count };
}

bool acpi::acpi_header_valid(const acpi::acpi_desc_header* header)
{
	uint8_t sum = 0;
	for (size_t i = 0; i < header->length; i++)
	{
		sum += ((char*)header)[i];
	}
	return sum == 0;
}