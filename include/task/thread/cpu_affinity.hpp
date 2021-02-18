#pragma once


#include "object/dispatcher.hpp"

#include "debug/nullability.hpp"
#include "debug/thread_annotations.hpp"

#include "kbl/lock/spinlock.h"
#include "kbl/data/list.hpp"
#include "kbl/data/name.hpp"
#include "kbl/checker/canary.hpp"

#include "ktl/shared_ptr.hpp"
#include "ktl/unique_ptr.hpp"
#include "ktl/string_view.hpp"
#include "ktl/concepts.hpp"
#include "kbl/lock/lock_guard.hpp"

#include "system/cls.hpp"
#include "system/time.hpp"
#include "system/deadline.hpp"

#include "drivers/apic/traps.h"

#include <compare>


namespace task
{
enum class [[clang::enum_extensibility(closed)]] cpu_affinity_type
{
	SOFT, HARD
};

struct cpu_affinity final
{
	cpu_num_type cpu;
	cpu_affinity_type type;

	auto operator<=>(const cpu_affinity&) const = default;
};

}