#if !defined(__DRIVERS_ACPI_V2_ACPI_V2_H)
#define __DRIVERS_ACPI_V2_ACPI_V2_H

#include "../acpi.h"

#include "system/error.hpp"

error_code init_xsdt(const acpi::acpi_rsdp *rsdp);

#endif // __DRIVERS_ACPI_V2_ACPI_V2_H
