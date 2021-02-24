#include "object/handle_table.hpp"

#include "task/process/process.hpp"

using namespace object;

kbl::intrusive_list<handle_entry,
                    lock::spinlock,
                    handle_entry::node_trait,
                    true> object::handle_table::global_handles_{};

template<std::convertible_to<dispatcher> T>
error_code_with_result<std::shared_ptr<T>> handle_table::object_from_handle(const handle_entry& h)
{
	return downcast_dispatcher<T>(h.ptr_);
}

void handle_table::add_handle(handle_entry_owner owner)
{
	lock::lock_guard g{ lock_ };
	add_handle_locked(std::move(owner));
}

void handle_table::add_handle_locked(handle_entry_owner owner)
{
	handles_.push_back(owner.release());
}

handle_entry_owner handle_table::remove_handle(handle_type h)
{
	lock::lock_guard g{ lock_ };
	return remove_handle_locked(h);
}

handle_entry_owner handle_table::remove_handle_locked(handle_type h)
{
	auto handle = get_handle_entry(h);
	if (!handle)
	{
		return nullptr;
	}

	return remove_handle_locked(handle);
}

handle_entry_owner handle_table::remove_handle_locked(handle_entry* e)
{
	if (e->link_.is_empty_or_detached())
	{
		return handle_entry_owner(e);
	}

	handles_.remove(e);
	return handle_entry_owner(e);
}

handle_entry* handle_table::get_handle_entry(handle_type h)
{
	auto[attr, idx] = PARSE_HANDLE(h);

	auto iter = handles_.begin();

	if ((attr & HATTR_LOCAL_PROC) == 0)
	{
		if (idx >= global_handles_.size())return nullptr;

		iter = global_handles_.begin();
	}
	else
	{
		if (idx >= handles_.size())return nullptr;
	}

	for (uint32_t i = 0; i < idx; i++, iter++);

	return &(*iter);
}


