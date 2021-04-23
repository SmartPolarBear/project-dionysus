#pragma once

#include "../../../../include/drivers/apic/traps.h"
#include "../../../../include/system/error.hpp"

error_code exception_divided_by_0([[maybe_unused]] trap::trap_frame info);

error_code exception_debug([[maybe_unused]] trap::trap_frame info);

error_code exception_no_maskable_interrupt([[maybe_unused]] trap::trap_frame info);

error_code exception_breakpoint([[maybe_unused]] trap::trap_frame info);

error_code exception_overflow([[maybe_unused]] trap::trap_frame info);

error_code exception_bound_range_exceeded([[maybe_unused]] trap::trap_frame info);

error_code exception_invalid_opcode([[maybe_unused]] trap::trap_frame info);

error_code exception_device_not_available([[maybe_unused]] trap::trap_frame info);

error_code exception_double_fault([[maybe_unused]] trap::trap_frame info);

error_code exception_invalid_tss([[maybe_unused]] trap::trap_frame info);

error_code exception_segment_np([[maybe_unused]] trap::trap_frame info);

error_code exception_stack_segment_fault([[maybe_unused]] trap::trap_frame info);

error_code exception_gpf([[maybe_unused]] trap::trap_frame info);

error_code exception_x87_floating_point([[maybe_unused]] trap::trap_frame info);

error_code exception_alignment_check([[maybe_unused]] trap::trap_frame info);

error_code exception_machine_check([[maybe_unused]] trap::trap_frame info);

error_code exception_SIMD_floating_point([[maybe_unused]] trap::trap_frame info);

error_code exception_virtualization([[maybe_unused]] trap::trap_frame info);

error_code exception_security([[maybe_unused]] trap::trap_frame info);

void install_exception_handles();

static inline constexpr bool is_exception(size_t trapnum)
{
	return (trapnum >= 0 && trapnum <= 20) || trapnum == 30;
}