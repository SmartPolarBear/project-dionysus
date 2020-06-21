#include "./traps.hpp"
#include "./exception.hpp"

#include "arch/amd64/x86.h"

#include "drivers/apic/apic.h"
#include "drivers/apic/traps.h"
#include "drivers/console/console.h"
#include "drivers/debug/kdebug.h"
#include "drivers/lock/spinlock.h"

#include "system/error.h"
#include "system/mmu.h"
#include "system/pmm.h"
#include "system/segmentation.hpp"
#include "system/vmm.h"

#include "libraries/libkernel/console/builtin_console.hpp"
#include <utility>

using std::pair;
using std::make_pair;

using exception_pair = pair<trap::processor_defined_traps, trap::trap_handle_func>;

using namespace trap;

exception_pair handles[] =
	{
		{ TRAP_DIVIDE, exception_divided_by_0 },
		{ TRAP_DEBUG, exception_debug },
		{ TRAP_NMI, exception_no_maskable_interrupt },
		{ TRAP_BRKPT, exception_breakpoint },
		{ TRAP_OFLOW, exception_overflow },
		{ TRAP_BOUND, exception_bound_range_exceeded },
		{ TRAP_ILLOP, exception_invalid_opcode },
		{ TRAP_DEVICE, exception_device_not_available },
		{ TRAP_DBLFLT, exception_double_fault },
		{ TRAP_TSS, exception_invalid_tss },
		{ TRAP_SEGNP, exception_segment_np },
		{ TRAP_STACK, exception_stack_segment_fault },
		{ TRAP_GPFLT, exception_gpf },
		{ TRAP_FPERR, exception_x87_floating_point },
		{ TRAP_ALIGN, exception_alignment_check },
		{ TRAP_MCHK, exception_machine_check },
		{ TRAP_SIMDERR, exception_SIMD_floating_point },
		{ TRAP_VIRTUALIZATION, exception_virtualization },
		{ TRAP_SECURITY, exception_security },
	};

void install_exception_handles()
{
	for (auto exp:handles)
	{
		trap_handle_register(exp.first, trap_handle{
			.handle=exp.second,
			.enable=true
		});
	}
}

error_code exception_divided_by_0([[maybe_unused]] trap::trap_frame info)
{
	KDEBUG_RICHPANIC("Divide-by-0 error.", "#DE", false, "");
	return ERROR_SUCCESS;
}

error_code exception_debug([[maybe_unused]] trap::trap_frame info)
{
	KDEBUG_RICHPANIC("Debug", "#DB", false, "");
	return ERROR_SUCCESS;
}

error_code exception_no_maskable_interrupt([[maybe_unused]] trap::trap_frame info)
{
	KDEBUG_RICHPANIC("NMI Interrupt", "", false, "");
	return ERROR_SUCCESS;
}

error_code exception_breakpoint([[maybe_unused]] trap::trap_frame info)
{
	KDEBUG_RICHPANIC("Breakpoint", "#BP", false, "");
	return ERROR_SUCCESS;
}

error_code exception_overflow([[maybe_unused]] trap::trap_frame info)
{
	KDEBUG_RICHPANIC("Overflow", "#OF", false, "");
	return ERROR_SUCCESS;
}

error_code exception_bound_range_exceeded([[maybe_unused]] trap::trap_frame info)
{
	KDEBUG_RICHPANIC("Bound range exceeded.", "#BR", false, "");
	return ERROR_SUCCESS;
}

error_code exception_invalid_opcode([[maybe_unused]] trap::trap_frame info)
{
	KDEBUG_RICHPANIC("Invalid opcode.", "#UD", false, "");
	return ERROR_SUCCESS;
}

error_code exception_device_not_available([[maybe_unused]] trap::trap_frame info)
{
	KDEBUG_RICHPANIC("Device not available", "#NM", false, "");
	return ERROR_SUCCESS;
}

error_code exception_double_fault([[maybe_unused]] trap::trap_frame info)
{
	KDEBUG_RICHPANIC("Double fault", "#DF", false, "");
	return ERROR_SUCCESS;
}

error_code exception_invalid_tss([[maybe_unused]] trap::trap_frame info)
{
	KDEBUG_RICHPANIC("Invalid TSS", "#TS", false, "Error=%lld\n", (size_t)info.err);
	return ERROR_SUCCESS;
}

error_code exception_segment_np([[maybe_unused]] trap::trap_frame info)
{
	KDEBUG_RICHPANIC("Segment not present", "#NP", false, "Error=%lld\n", (size_t)info.err);
	return ERROR_SUCCESS;
}

error_code exception_stack_segment_fault([[maybe_unused]] trap::trap_frame info)
{
	KDEBUG_RICHPANIC("Stack segment fault", "#SS", false, "Error=%lld\n", (size_t)info.err);
	return ERROR_SUCCESS;
}

error_code exception_gpf([[maybe_unused]] trap::trap_frame info)
{
	KDEBUG_RICHPANIC("General protection fault", "#GP", false, "Error=%lld\n", (size_t)info.err);
	return ERROR_SUCCESS;
}

error_code exception_x87_floating_point([[maybe_unused]] trap::trap_frame info)
{
	KDEBUG_RICHPANIC("x87 floating-point exception", "#MF", false, "");
	return ERROR_SUCCESS;
}

error_code exception_alignment_check([[maybe_unused]] trap::trap_frame info)
{
	KDEBUG_RICHPANIC("Alignment check", "#AC", false, "Error=%lld\n", (size_t)info.err);
	return ERROR_SUCCESS;
}

error_code exception_machine_check([[maybe_unused]] trap::trap_frame info)
{
	KDEBUG_RICHPANIC("Machine check", "#MC", false, "");
	return ERROR_SUCCESS;
}

error_code exception_SIMD_floating_point([[maybe_unused]] trap::trap_frame info)
{
	KDEBUG_RICHPANIC("SIMD floating-point exception", "#XF", false, "");
	return ERROR_SUCCESS;
}

error_code exception_virtualization([[maybe_unused]] trap::trap_frame info)
{
	KDEBUG_RICHPANIC("Virtualization exception", "#VE", false, "");
	return ERROR_SUCCESS;
}

error_code exception_security([[maybe_unused]] trap::trap_frame info)
{
	KDEBUG_RICHPANIC("Security exception", "#SX", false, "Error=%lld\n", (size_t)info.err);
	return ERROR_SUCCESS;
}


