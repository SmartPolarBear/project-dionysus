#pragma once

#include "system/types.h"
#include "system/error.hpp"

error_code apic_error_trap_handle([[maybe_unused]] trap::trap_frame frame);

error_code spurious_trap_handle([[maybe_unused]] trap::trap_frame info);