#include "object/handle_table.hpp"

#include "task/process/process.hpp"

using namespace object;
using namespace lock;

handle_table::handle_table() : local_{ true }, parent_{ nullptr }
{

}

handle_table::handle_table(global_handle_table_tag)
	: local_{ false }, parent_{ nullptr }
{
	table_cache_ = memory::kmem::kmem_cache_create("handle_table", sizeof(table));

	initialize_table();
}

handle_table::handle_table(dispatcher* parent) : local_{ true }, parent_{ parent }
{
	table_cache_ = memory::kmem::kmem_cache_create("handle_table", sizeof(table));

	initialize_table();
}

void handle_table::initialize_table()
{
	root_.next[0] = new(memory::kmem::kmem_cache_alloc(table_cache_)) table{};
	root_.next[0]->next[0] = new(memory::kmem::kmem_cache_alloc(table_cache_)) table{};
	root_.next[0]->next[0]->next[0] = new(memory::kmem::kmem_cache_alloc(table_cache_)) table{};
	root_.next[0]->next[0]->next[0]->entry[0] = nullptr;

	memset(&next_, 0, sizeof(next_));
}

error_code_with_result<std::tuple<size_t, size_t, size_t, size_t>> handle_table::first_free()
{
	for (size_t l1 = 0; l1 < next_.l1; l1++)
	{
		for (size_t l2 = 0; l2 < next_.l2; l2++)
		{
			for (size_t l3 = 0; l3 < next_.l3; l3++)
			{
				for (size_t l4 = 0; l4 < next_.l4; l4++)
				{
					if (!root_.next[l1]->next[l2]->next[l3]->entry[l4])
					{
						return std::make_tuple(l1, l2, l3, l4);
					}
				}
			}
		}
	}
	auto[l1, l2, l3, l4]=next_;
	if (auto err = increase_next();err != ERROR_SUCCESS)
	{
		return err;
	}

	return std::make_tuple(l1, l2, l3, l4);
}

handle_type handle_table::add_handle(handle_entry_owner owner)
{
	lock::lock_guard g{ lock_ };
	return add_handle_locked(std::move(owner));
}

handle_type handle_table::add_handle_locked(handle_entry_owner owner)
{
	KDEBUG_ASSERT(owner);

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
	KDEBUG_ASSERT(owner.get());

	handle_entry* ptr = nullptr;
	if (auto local_value = local_get_locked(owner.get());!local_value)
	{
		ptr = owner.release();

		if (auto find_res = allocate_slot();!has_error(find_res))
		{
			auto[l1, l2, l3, l4]= get_result(find_res);
			root_.next[l1]->next[l2]->next[l3]->entry[l4] = ptr;
			ptr->value_ = MAKE_HANDLE(attr, l1, l2, l3, l4);
		}
		else
		{
			KDEBUG_GERNERALPANIC_CODE(get_error_code(find_res));
		}
	}
	else
	{
		ptr = owner.release();
	}

	return ptr->value_;
}

handle_type handle_table::entry_to_handle(handle_entry* ptr) const
{
//	uint16_t attr = 0;
//
//	if (local_)attr |= HATTR_LOCAL_PROC;
//	else attr |= HATTR_GLOBAL;
//	return MAKE_HANDLE(attr, (uintptr_t)&ptr->link_);
	return ptr->value_;
}

handle_entry_owner handle_table::remove_handle(handle_type h)
{
	lock::lock_guard g{ lock_ };
	return remove_handle_locked(h);
}

handle_entry_owner handle_table::remove_handle(handle_entry* e)
{
	lock::lock_guard g{ lock_ };
	return remove_handle_locked(e);
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

	if (!local_exist_locked(e))
	{
		return handle_entry_owner(e);
	}

	for (size_t l1 = 0; l1 < next_.l1; l1++)
	{
		for (size_t l2 = 0; l2 < next_.l2; l2++)
		{
			for (size_t l3 = 0; l3 < next_.l3; l3++)
			{
				for (size_t l4 = 0; l4 < next_.l4; l4++)
				{
					auto slot = root_.next[l1]->next[l2]->next[l3]->entry[l4];
					if (slot && slot->ptr_ == e->ptr_)
					{
						root_.next[l1]->next[l2]->next[l3]->entry[l4] = nullptr;
					}
				}
			}
		}
	}

//	handles_.remove(e);

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
	auto[attr, l1, l2, l3, l4] = DISASSEMBLE_HANDLE(h);

	if ((attr & HATTR_GLOBAL) && local_)return nullptr;

	KDEBUG_ASSERT(l1 <= next_.l1);
	KDEBUG_ASSERT(l2 <= next_.l2);
	KDEBUG_ASSERT(l3 <= next_.l3);
	KDEBUG_ASSERT(l4 <= next_.l4);

	return root_.next[l1]->next[l2]->next[l3]->entry[l4];
}

bool handle_table::local_exist_locked(handle_entry* owner) TA_REQ(lock_)
{
	for (size_t l1 = 0; l1 < next_.l1; l1++)
	{
		for (size_t l2 = 0; l2 < next_.l2; l2++)
		{
			for (size_t l3 = 0; l3 < next_.l3; l3++)
			{
				for (size_t l4 = 0; l4 < next_.l4; l4++)
				{
					auto slot = root_.next[l1]->next[l2]->next[l3]->entry[l4];
					if (slot && slot->ptr_ == owner->ptr_)
					{
						return true;
					}
				}
			}
		}
	}

	return false;
}

std::optional<std::tuple<size_t, size_t, size_t, size_t>> handle_table::local_get_locked(handle_entry* owner) TA_REQ(
	lock_)
{
	for (size_t l1 = 0; l1 < next_.l1; l1++)
	{
		for (size_t l2 = 0; l2 < next_.l2; l2++)
		{
			for (size_t l3 = 0; l3 < next_.l3; l3++)
			{
				for (size_t l4 = 0; l4 < next_.l4; l4++)
				{
					auto slot = root_.next[l1]->next[l2]->next[l3]->entry[l4];
					if (slot && slot->ptr_ == owner->ptr_)
					{
						return std::make_tuple(l1, l2, l3, l4);
					}
				}
			}
		}
	}

	return std::nullopt;
}

void handle_table::clear()
{

	for (size_t l1 = 0; l1 < next_.l1; l1++)
	{
		for (size_t l2 = 0; l2 < next_.l2; l2++)
		{
			for (size_t l3 = 0; l3 < next_.l3; l3++)
			{
				for (size_t l4 = 0; l4 < next_.l4; l4++)
				{
					auto slot = root_.next[l1]->next[l2]->next[l3]->entry[l4];
					handle_entry_owner discard{ slot };
					// the deleter is called
				}
				memory::kmem::kmem_cache_free(table_cache_, root_.next[l1]->next[l2]->next[l3]);
			}
			memory::kmem::kmem_cache_free(table_cache_, root_.next[l1]->next[l2]);
		}
		memory::kmem::kmem_cache_free(table_cache_, root_.next[l1]);
	}

	initialize_table(); // reinitialize the first slot
}

handle_entry* handle_table::query_handle_by_name(ktl::string_view name)
{
	lock_guard g{ lock_ };
	return query_handle_by_name_locked(name);
}

handle_entry* handle_table::query_handle_by_name_locked(ktl::string_view name) TA_REQ(lock_)
{

	for (size_t l1 = 0; l1 < next_.l1; l1++)
	{
		for (size_t l2 = 0; l2 < next_.l2; l2++)
		{
			for (size_t l3 = 0; l3 < next_.l3; l3++)
			{
				for (size_t l4 = 0; l4 < next_.l4; l4++)
				{
					auto slot = root_.next[l1]->next[l2]->next[l3]->entry[l4];
					if (slot && name.compare(slot->name_.data()) == 0)
					{
						return slot;
					}
				}
			}
		}
	}

	return nullptr;
}

std::tuple<int, int> handle_table::increase_next_cur(size_t value)
{
	auto new_value = value + 1;
	return { new_value >= MAX_HANDLE_PER_TABLE, new_value % MAX_HANDLE_PER_TABLE };
}

error_code handle_table::increase_next()
{
	if (auto[c4, v4] = increase_next_cur(next_.l4);c4)
	{
		if (auto[c3, v3]=increase_next_cur(next_.l3);c3)
		{
			if (auto[c2, v2]=increase_next_cur(next_.l2);c2)
			{
				if (auto[c1, v1]=increase_next_cur(next_.l1);c1)
				{
					next_.l1 = v1;
					return -ERROR_TOO_MANY_HANDLES;
				}
				else
				{
					next_.l1 = v1;
				}
				next_.l2 = v2;
			}
			else
			{
				next_.l2 = v2;
			}
			next_.l3 = v3;
		}
		else
		{
			next_.l3 = v3;
		}
		next_.l4 = v4;
	}
	else
	{
		next_.l4 = v4;
	}

	return ERROR_SUCCESS;
}
error_code_with_result<std::tuple<size_t, size_t, size_t, size_t>> handle_table::allocate_slot()
{
	auto free = first_free();
	if (has_error(free))
	{
		return free;
	}

	if (auto err = increase_next();err != ERROR_SUCCESS)
	{
		return err;
	}

	return free;
}



