#include "object/handle_table.hpp"

#include "task/process/process.hpp"

using namespace object;
using namespace lock;

kbl::intrusive_list<handle_entry,
                    lock::spinlock,
                    handle_entry::node_trait,
                    true> object::handle_table::global_handles_{};

lock::spinlock object::handle_table::global_lock_{ "ghandle" };

template<std::convertible_to<dispatcher> T>
error_code_with_result<std::shared_ptr<T>> handle_table::object_from_handle(const handle_entry& h)
{
	return downcast_dispatcher<T>(h.ptr_);
}

handle_type handle_table::add_handle(handle_entry_owner owner)
{
	lock::lock_guard g{ lock_ };
	return add_handle_locked(std::move(owner));
}

handle_type handle_table::add_handle_locked(handle_entry_owner owner)
{
	if (!local_exist_locked(owner.get()))
	{
		handles_.push_back(owner.release());
	}
	else
	{
		[[maybe_unused]] auto discard = owner.release();
	}
	return MAKE_HANDLE(HATTR_LOCAL_PROC, (uintptr_t)&owner->link_);
}

handle_entry_owner handle_table::remove_handle(handle_type h)
{
	lock::lock_guard g{ lock_ };
	return remove_handle_locked(h);
}

handle_entry_owner handle_table::remove_handle_locked(handle_type h)
{
	auto handle = get_handle_entry_locked(h);
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
	lock_guard g1{ lock_ };

	return get_handle_entry_locked(h);
}

handle_entry* handle_table::get_handle_entry_locked(handle_type h)
{
	auto[attr, idx] = PARSE_HANDLE(h);

	if ((attr & HATTR_LOCAL_PROC) == 0)
	{
		if (idx >= global_handles_.size())return nullptr;

		auto link_ptr = *reinterpret_cast<decltype(global_handles_)::head_type*>(INDEX_TO_ADDR(idx));
		return link_ptr.parent_;

	}
	else
	{
		if (idx >= handles_.size())return nullptr;
		auto link_ptr = *reinterpret_cast<decltype(handles_)::head_type*>(INDEX_TO_ADDR(idx));
		return link_ptr.parent_;
	}

	return nullptr;
}

bool handle_table::local_exist_locked(handle_entry* owner) TA_REQ(lock_)
{
	for (auto& handle:handles_)
	{
		if (handle.ptr_ == owner->ptr_)return true;
	}

	return false;
}



