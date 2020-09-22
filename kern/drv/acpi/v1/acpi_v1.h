#if !defined(__DRIVERS_ACPI_V1_ACPI_V1_H)
#define __DRIVERS_ACPI_V1_ACPI_V1_H

#include "../acpi.h"
#include "system/error.hpp"

error_code init_rsdt(const acpi::acpi_rsdp *rsdp);

#endif // __DRIVERS_ACPI_V1_ACPI_V1_H
