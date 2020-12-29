#pragma once

#include "object/kernel_object.hpp"

#include "arch/amd64/cpu/regs.h"

#include "system/mmu.h"
#include "system/types.h"
#include "system/vmm.h"
#include "system/syscall.h"
#include "system/messaging.hpp"

#include "drivers/apic/traps.h"
#include "kbl/lock/spinlock.h"

#include "kbl/data/pod_list.h"
#include "kbl/data/list_base.hpp"

#include "ktl/mutex/lock_guard.hpp"
#include "kbl/atomic/atomic_ref.hpp"
#include "kbl/ref_count/ref_count_base.hpp"

#include <cstring>
#include <algorithm>
#include <span>
#include <optional>

namespace task
{
	using right_type = uint64_t;

	template<typename TDispatcher, right_type default_rights>
	class dispatcher
		: public object::kernel_object
	{
	 public:
		virtual ~dispatcher() = default;

	 protected:
		[[nodiscard]]lock::spinlock* get_lock()
		{
			return &lock;
		}
		mutable lock::spinlock lock;
	};
}