#pragma once

#include "object/object.hpp"

#include "system/types.h"

#include "kbl/lock/spinlock.h"

#include "kbl/data/list_base.hpp"

namespace object
{
using right_type = uint64_t;

template<typename TDispatcher, right_type default_rights>
class dispatcher
	: public object
{
 public:
	virtual ~dispatcher() = default;

 protected:
	[[nodiscard]]lock::spinlock* get_lock()
	{
		return &lock;
	}

	mutable lock::spinlock lock{ "dispatcher" };
};
}