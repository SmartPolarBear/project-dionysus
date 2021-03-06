#include "include/traps.hpp"
#include "include/exception.hpp"

#include "arch/amd64/cpu/x86.h"

#include "drivers/apic/apic.h"
#include "drivers/apic/traps.h"
#include "drivers/console/console.h"
#include "debug/kdebug.h"
#include "kbl/lock/spinlock.h"

#include "system/error.hpp"
#include "system/mmu.h"
#include "system/pmm.h"
#include "system/segmentation.hpp"
#include "system/vmm.h"
#include "task/process/process.hpp"


#include "../../libs/basic_io/include/builtin_text_io.hpp"
#include <cstring>

error_code msi_base_trap_handle([[maybe_unused]]trap::trap_frame tf)
{
	return ERROR_SUCCESS;
}