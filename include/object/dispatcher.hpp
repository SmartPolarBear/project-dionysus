#pragma once

#include "object/object.hpp"
#include "object/ref_counted.hpp"

#include "system/types.h"

#include "kbl/lock/spinlock.h"

namespace object
{
using right_type = uint64_t;

template<typename TDispatcher, right_type default_rights>
class dispatcher
	: public ref_counted,
	  public object<TDispatcher>
{
 public:
	constexpr dispatcher() = default;

	virtual ~dispatcher() = default;

 protected:
	[[nodiscard]]lock::spinlock* get_lock()
	{
		return &lock;
	}

	mutable lock::spinlock lock{ "dispatcher" };

};
}