#include "object/handle_table.hpp"

#include "task/process/process.hpp"

using namespace object;
using namespace lock;

handle_table handle_table::global_handle_table_{ create_global_handle_table };

handle_table* handle_table::get_global_handle_table()
{
	return &global_handle_table_;
}

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
	if (local_ && cur_proc.is_valid() && cur_proc != nullptr)
	{
		owner->owner_process_id = cur_proc->get_koid();
	}
	else if (local_ && parent_)
	{
		owner->owner_process_id = parent_->get_koid();
	}

	owner->parent_ = this;

	uint16_t attr = 0;

	if (local_)attr |= HATTR_LOCAL_PROC;
	else attr |= HATTR_GLOBAL;

	handle_entry* ptr = nullptr;
	if (!local_exist_locked(owner.get()))
	{
		handles_.push_back(ptr = owner.release());
	}
	else
	{
		ptr = owner.release();
	}

	return MAKE_HANDLE(attr, (uintptr_t)&ptr->link_);
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
	e->owner_process_id = -1;
	e->parent_ = nullptr;

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

	if ((attr & HATTR_GLOBAL) && local_)return nullptr;

	auto link_ptr = *reinterpret_cast<decltype(handles_)::head_type*>(INDEX_TO_ADDR(idx));
	return link_ptr.parent_;
}

bool handle_table::local_exist_locked(handle_entry* owner) TA_REQ(lock_)
{
	for (auto& handle:handles_)
	{
		if (handle.ptr_ == owner->ptr_)return true;
	}

	return false;
}

void handle_table::clear()
{
	for (auto& handle:handles_)
	{
		handle_entry_owner discard{ &handle };
		// the deleter is called
	}

	handles_.clear();
}

handle_entry* handle_table::query_handle_by_name(ktl::string_view name)
{
	lock_guard g{ lock_ };
	return query_handle_by_name_locked(name);
}

handle_entry* handle_table::query_handle_by_name_locked(ktl::string_view name) TA_REQ(lock_)
{
	for (auto& handle:handles_)
	{
		if (name.compare(handle.name_.data()) == 0)
		{
			return &handle;
		}
	}
	return nullptr;
}

template<typename T>
handle_entry* handle_table::query_handle(T&& pred)
{
	lock_guard g{ lock_ };
	return query_handle_locked(std::forward(pred));
}

template<typename T>
handle_entry* handle_table::query_handle_locked(T&& pred) TA_REQ(lock_)
{
	for (auto& handle:handles_)
	{
		if (pred(handle))
		{
			return &handle;
		}
	}
	return nullptr;
}




