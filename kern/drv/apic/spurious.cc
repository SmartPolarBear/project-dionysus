#include "system/types.h"
#include "system/error.hpp"

#include "drivers/apic/traps.h"
#include "drivers/apic/local_apic.hpp"

#include "debug/kdebug.h"

using namespace apic;
using namespace local_apic;



error_code spurious_trap_handle([[maybe_unused]] trap::trap_frame info)
{
	return ERROR_SUCCESS;
}