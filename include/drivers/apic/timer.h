#pragma once

#include "system/types.h"

namespace timer
{

PANIC void init_apic_timer();

uint64_t get_ticks();

/// \brief mask current cpu
/// \param masked
void mask_cpu_local_timer(bool masked);

/// \brief mask specified cpu
/// \param cpuid
/// \param masked
void mask_cpu_local_timer(size_t cpuid, bool masked);
} // namespace timer

