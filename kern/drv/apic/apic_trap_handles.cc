#include "system/types.h"
#include "system/error.hpp"

#include "drivers/apic/traps.h"
#include "drivers/apic/local_apic.hpp"

#include "debug/kdebug.h"

using namespace apic;
using namespace local_apic;

error_code apic_error_trap_handle([[maybe_unused]] trap::trap_frame frame)
{
	auto esr = read_lapic<_internals::error_status_reg>(ESR_ADDR);

	KDEBUG_RICHPANIC("Interrupted by an APIC error.", "APIC ERROR", false, "error bits: %d", esr.error_bits);

	return ERROR_SUCCESS;
}

error_code spurious_trap_handle([[maybe_unused]] trap::trap_frame info)
{
	return ERROR_SUCCESS;
}