#pragma once

#include "system/types.h"

#include "drivers/debug/kdebug.h"

// Intel's advanced configuration and power interface
namespace acpi
{

constexpr const char *SIGNATURE_RSDP = "RSD PTR ";

enum rsdp_reversion : uint8_t
{
    RSDP_REV1 = 0,
    RSDP_REV2 = 2
};

// root system description pointer
struct acpi_rsdp
{
    uint8_t signature[8];
    uint8_t checksum;
    uint8_t oem_id[6];
    uint8_t revision;
    uint32_t rsdt_addr_phys;

    uint32_t length;
    uint64_t xsdt_addr_phys;
    uint8_t xchecksum;
    uint8_t reserved[3];
} __attribute__((packed));

// 5.2.6
struct acpi_desc_header
{
    uint8_t signature[4];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    uint8_t oem_id[6];
    uint8_t oem_tableid[8];
    uint32_t oem_revision;
    uint8_t creator_id[4];
    uint32_t creator_revision;
} __attribute__((__packed__));

// 5.2.7
// root system descriptor table
struct acpi_rsdt
{
    acpi_desc_header header;
    uint32_t entry[0];
} __attribute__((__packed__));

// 5.2.7
// eXtended system descriptor table
struct acpi_xsdt
{
    acpi_desc_header header;
    uintptr_t entry[0];
} __attribute__((__packed__));

enum madt_entry_type : uint8_t
{
    MADT_ENTRY_LAPIC = 0,
    MADT_ENTRY_IOAPIC = 1,
    MADT_ENTRY_ISO = 2,
    MADT_ENTRY_NMI = 3,

    MADT_ENTRY_LAPIC_NMI = 4,
    MADT_ENTRY_LAPIC_ADDR_OVERRIDE = 5,
    MADT_ENTRY_IO_SAPIC = 6,

    MADT_ENTRY_LSAPIC = 7,
    MADT_ENTRY_PIS = 8,
    MADT_ENTRY_Lx2APIC = 9,
    MADT_ENTRY_Lx2APIC_NMI = 0xA,
    MADT_ENTRY_GIC = 0xB,
    MADT_ENTRY_GICD = 0xC,
};

// 5.2.12
// multiple APIC description table
constexpr const char *SIGNATURE_MADT = "APIC";
struct acpi_madt
{
    struct acpi_desc_header header;
    uint32_t lapic_addr_phys;
    uint32_t flags;
    uint8_t table[0];
} __attribute__((__packed__));

struct madt_entry_header
{
    madt_entry_type type;
    uint8_t length;
} __attribute__((__packed__));
static_assert(sizeof(madt_entry_header) == sizeof(char[2]));

// 5.2.12.2
constexpr uint32_t APIC_LAPIC_ENABLED = 1;
struct madt_lapic
{
    uint8_t type;
    uint8_t length;
    uint8_t acpi_id;
    uint8_t apic_id;
    uint32_t flags;
} __attribute__((__packed__));

// 5.2.12.3
struct madt_ioapic
{
    uint8_t type;
    uint8_t length;
    uint8_t id;
    uint8_t reserved;
    uint32_t addr;
    uint32_t interrupt_base;
} __attribute__((__packed__));

struct madt_iso
{
    uint8_t type;
    uint8_t length;
    uint8_t bus_src;
    uint8_t irq_src;
    uint32_t global_system_interrupt;
    uint16_t flags;
} __attribute__((__packed__));

PANIC void init_acpi(void);

// size_t get_ioapic_count(void);
// void get_ioapics(madt_ioapic res[], size_t bufsz);
// madt_ioapic get_first_ioapic(void);

// when ret==nullptr, this returns the count of ioapic descriptors
size_t get_ioapic_descriptors(size_t bufsz, OUT madt_ioapic **buf);

static inline PANIC madt_ioapic *first_ioapic(void)
{
    madt_ioapic *buf[2] = {nullptr, nullptr};
    size_t sz = get_ioapic_descriptors(1, buf);
    KDEBUG_ASSERT(sz == 1);
    return buf[0];
}

// size_t get_iso_count(void);
// void get_isos(madt_iso res[], size_t bufsz);

size_t get_intr_src_override_descriptors(size_t bufsz, OUT madt_iso **buf);

// instead of copy madt_lapic to cpus array, directly provide interface to get them

} // namespace acpi
