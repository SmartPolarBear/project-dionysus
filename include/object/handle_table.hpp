#pragma once

#include "kbl/data/list.hpp"

#include "kbl/lock/spinlock.h"

namespace object
{
class handle_table final
{
 public:

 private:
	kbl::intrusive_list<handle,
	                    lock::spinlock,
	                    handle::node_trait,
	                    true> handles_{};
};
}