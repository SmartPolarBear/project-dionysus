#pragma once

#include "system/types.h"

namespace timer
{
	PANIC void setup_apic_timer(void);

	PANIC void init_apic_timer(void);

	void set_enable_on_cpu(size_t cpuid,bool enable);
} // namespace timer

