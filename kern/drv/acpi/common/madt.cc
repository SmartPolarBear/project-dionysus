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

#include <cstring>
#include <algorithm>

using acpi::acpi_desc_header;
using acpi::acpi_madt;
using acpi::acpi_rsdp;
using acpi::acpi_rsdt;
using acpi::acpi_xsdt;
using acpi::madt_entry_header;
using acpi::madt_ioapic;
using acpi::madt_iso;

using trap::TRAP_NUMBERMAX;

using std::min;
using std::max;

using namespace apic;

madt_ioapic ioapics[CPU_COUNT_LIMIT] = {};
size_t ioapic_count = 0;

madt_iso intr_src_overrides[TRAP_NUMBERMAX] = {};
size_t iso_count = 0;

[[nodiscard]] error_code acpi_madt_init(const acpi_madt* madt)
{
	// KDEBUG_ASSERT(madt != nullptr);
	// KDEBUG_ASSERT(madt->header.length >= sizeof(*madt));

	if (madt == nullptr)
	{
		return -ERROR_INVALID;
	}

	if (madt->header.length < sizeof(*madt))
	{
		return -ERROR_INVALID;
	}

	if (!acpi_header_valid(&madt->header))
	{
		return -ERROR_INVALID;
	}

	local_apic::lapic_base = IO2V<decltype(local_apic::lapic_base)>((void*)static_cast<uintptr_t>(madt->lapic_addr_phys));

	const madt_entry_header* begin = reinterpret_cast<decltype(begin)>(madt->table),
		* end = reinterpret_cast<decltype(begin)>(madt->table + madt->header.length - sizeof(*madt));

	auto next_entry = [](auto entry)
	{
	  return reinterpret_cast<decltype(entry)>((void*)(uintptr_t(entry) + entry->length));
	};

	for (auto entry = const_cast<madt_entry_header*>(begin);
		 entry != end;
		 entry = next_entry(entry))
	{
		switch (entry->type)
		{
		case acpi::MADT_ENTRY_LAPIC:
		{
			acpi::madt_lapic* lapic = reinterpret_cast<decltype(lapic)>(entry);
			if (sizeof(*lapic) != lapic->length)
			{
				return -ERROR_INVALID;
			}

			if (!(lapic->flags & acpi::APIC_LAPIC_ENABLED))
			{
				break;
			}

			cpus[cpu_count].id = cpu_count;
			cpus[cpu_count].apicid = lapic->apic_id;
			cpus[cpu_count].present = true;

			cpu_count++;
			break;
		}
		case acpi::MADT_ENTRY_IOAPIC:
		{
			acpi::madt_ioapic* ioapic = reinterpret_cast<decltype(ioapic)>(entry);

			if (sizeof(*ioapic) != ioapic->length)
			{
				return -ERROR_INVALID;
			}

			ioapics[ioapic_count] = madt_ioapic{ *ioapic };
			ioapic_count++;
			break;
		}
		case acpi::MADT_ENTRY_ISO:
		{
			acpi::madt_iso* iso = reinterpret_cast<decltype(iso)>(entry);
			// KDEBUG_ASSERT(sizeof(*iso) == iso->length);
			if (sizeof(*iso) != iso->length)
			{
				return -ERROR_INVALID;
			}
			intr_src_overrides[iso_count] = madt_iso{ *iso };
			iso_count++;
			break;
		}
		default:
			break;
		}
	}

	// the kernel must run with at lease 2 CPUs
	if (cpu_count < 2)
	{
		return -ERROR_HARDWARE_NOT_COMPATIBLE;
	}

	if (ioapic_count < 1)
	{
		return -ERROR_INVALID;
	}

	return ERROR_SUCCESS;
}

size_t acpi::get_ioapic_descriptors(size_t bufsz, OUT madt_ioapic** buf)
{
	if (buf == nullptr)
	{
		return ioapic_count;
	}

	size_t cpy_count = min(bufsz, ioapic_count);
	for (size_t i = 0; i < cpy_count; i++)
	{
		buf[i] = &ioapics[i];
	}

	return cpy_count;
}

size_t get_intr_src_override_descriptors(size_t bufsz, OUT madt_iso** buf)
{
	if (buf == nullptr)
	{
		return iso_count;
	}

	size_t cpy_count = std::min(bufsz, ioapic_count);
	for (size_t i = 0; i < cpy_count; i++)
	{
		buf[i] = &intr_src_overrides[i];
	}

	return cpy_count;
}
