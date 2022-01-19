#include "system/types.h"
#include "system/kom.hpp"

#include "object/handle_entry.hpp"
#include "object/handle_table.hpp"

#include "object/object_manager.hpp"

#include "kbl/checker/allocate_checker.hpp"

using namespace kbl;

using namespace object;

handle_table* object::object_manager::global_handle_table_{ nullptr };

object::handle_table* object::object_manager::global_handles()
{
	KDEBUG_ASSERT(global_handle_table_);
	return global_handle_table_;
}

void object::init_object_manager()
{
	KDEBUG_ASSERT(!object_manager::global_handle_table_);

	allocate_checker ck{};
	object_manager::global_handle_table_ = new(&ck) handle_table{ create_global_handle_table };

	if(!ck.check())
	{
		KDEBUG_GENERALPANIC("Cannot allocate the global handle table");
	}
}
