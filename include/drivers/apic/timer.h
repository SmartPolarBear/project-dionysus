#pragma once

#include "system/types.h"

namespace timer
{
	PANIC void setup_apic_timer();

	PANIC void init_apic_timer();

	uint64_t get_ticks();

	void set_enable_on_cpu(size_t cpuid, bool enable);
} // namespace timer

