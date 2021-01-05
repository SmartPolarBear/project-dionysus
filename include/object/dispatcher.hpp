#pragma once

#include "kernel_object.hpp"

#include "../../arch/amd64/include/arch/amd64/cpu/regs.h"

#include "../system/mmu.h"
#include "../system/types.h"
#include "../system/vmm.h"
#include "../system/syscall.h"
#include "../system/messaging.hpp"

#include "../drivers/apic/traps.h"
#include "../../kern/libs/kbl/include/kbl/lock/spinlock.h"

#include "../../kern/libs/kbl/include/kbl/data/pod_list.h"
#include "../../kern/libs/kbl/include/kbl/data/list_base.hpp"

#include "../../kern/libs/ktl/include/ktl/mutex/lock_guard.hpp"
#include "../../kern/libs/kbl/include/kbl/atomic/atomic_ref.hpp"
#include "../../kern/libs/kbl/include/kbl/ref_count/ref_count_base.hpp"

#include "../../build/external/third_party_root/include/c++/v1/cstring"
#include "../../build/external/third_party_root/include/c++/v1/algorithm"
#include "../../build/external/third_party_root/include/c++/v1/span"
#include "../../build/external/third_party_root/include/c++/v1/optional"

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