#pragma once

#include "kbl/data/list.hpp"

#include "kbl/lock/spinlock.h"

#include "object/handle.hpp"

namespace object
{
class handle_table final
{
 public:
	template<typename T>
	error_code_with_result<T> object_from_handle(handle h);
 private:
	kbl::intrusive_list<handle,
	                    lock::spinlock,
	                    handle::node_trait,
	                    true> handles_{};
};
}