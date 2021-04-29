#include "arch/amd64/cpu/regs.h"

#include "system/error.hpp"
#include "system/scheduler.h"
#include "system/types.h"

#include "drivers/apic/timer.h"

#include "drivers/acpi/cpu.h"
#include "drivers/apic/apic.h"
#include "drivers/apic/traps.h"
#include "debug/kdebug.h"
#include "kbl/lock/spinlock.h"
#include "drivers/acpi/cpu.h"

#include "task/scheduler/scheduler.hpp"
#include "task/thread/thread.hpp"
#include "task/process/process.hpp"

#include "ktl/atomic.hpp"

#include "builtin_text_io.hpp"

using namespace apic;

using namespace local_apic;

using namespace lock;

using trap::IRQ_TIMER;
using trap::TRAP_IRQ0;

volatile ktl::atomic<uint64_t> ticks{ 0 };
static_assert(ktl::atomic<uint64_t>::is_always_lock_free);

volatile ktl::atomic<uint64_t> timer_mask{ 0 };
static_assert(ktl::atomic<uint64_t>::is_always_lock_free);

// defined below
error_code trap_handle_tick(trap::trap_frame info);

PANIC void timer::init_apic_timer()
{
	// register the handle
	trap::trap_handle_register(trap::IRQ_TO_TRAPNUM(IRQ_TIMER),
		trap::trap_handle
			{
				.handle = trap_handle_tick,
				.enable = true
			});

}

error_code trap_handle_tick([[maybe_unused]] trap::trap_frame info)
{
	size_t id = cpu->id;

	if (!(timer_mask.load() & (1 << id)))
	{
		ticks++;
		local_apic::write_eoi();

		task::global_thread_lock.assert_not_held();
		task::scheduler::current::timer_tick_handle();
	}

	return ERROR_SUCCESS;
}

void timer::mask_cpu_local_timer(bool masked)
{
	mask_cpu_local_timer(cpu->id, masked);
}

void timer::mask_cpu_local_timer(size_t cpuid, bool masked)
{
	if (masked)
	{
		timer_mask.fetch_or(1ull << cpuid);
	}
	else
	{
		timer_mask.fetch_add(~(1ull << cpuid));
	}
}

uint64_t timer::get_ticks()
{
	return ticks.load(ktl::memory_order_seq_cst);
}
