#include "system/types.h"
#include "system/kom.hpp"

#include "object/handle_entry.hpp"
#include "object/handle_table.hpp"
#include "object/object_manager.hpp"

#include "task/process/process.hpp"

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

	if (!ck.check())
	{
		KDEBUG_GENERALPANIC("Cannot allocate the global handle table");
	}
}

handle_type object_manager::get_global_handle(handle_type local)
{
	if (!HANDLE_HAS_ATTRIBUTE(local, HATTR_LOCAL_PROC))
	{
		return local;
	}

	auto entry = cur_proc->handle_table()->get_handle_entry(local);

	if (!entry)
	{
		int a = 0;
		a = 10;
	}

	KDEBUG_ASSERT(entry);

	if (auto query_ret = global_handle_table_->query_handle([&entry](const handle_entry& h)
		{
		  return h.object() && entry->object() && h.object()->get_koid() == entry->object()->get_koid();
		});query_ret)
	{
		return global_handle_table_->entry_to_handle(query_ret);
	}

	return global_handle_table_->add_handle(handle_entry_owner{ entry });
}

handle_entry* object_manager::get_handle_entry(handle_type handle)
{
	if (!HANDLE_HAS_ATTRIBUTE(handle, HATTR_LOCAL_PROC))
	{
		return cur_proc->handle_table()->get_handle_entry(handle);
	}

	return global_handle_table_->get_handle_entry(handle);

}

handle_entry* object_manager::get_global_handle_entry(handle_type handle)
{
	handle = get_global_handle(handle);
	return global_handle_table_->get_handle_entry(handle);
}
