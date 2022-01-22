#pragma once

#include "kbl/data/list.hpp"
#include "kbl/lock/spinlock.h"
#include "object/dispatcher.hpp"

#include "ktl/concepts.hpp"

#include "object/handle_entry.hpp"

#include "object/public/handle_type.hpp"

#include <optional>

namespace object
{

enum [[clang::flag_enum]] handle_type_attributes : uint16_t
{
	HATTR_GLOBAL = 0b1,
	HATTR_LOCAL_PROC = 0b10,
};

struct global_handle_table_tag
{
};

static inline constexpr global_handle_table_tag create_global_handle_table;

union handle
{
	handle_type handle_value;
	struct
	{
		uint64_t l4: 8;
		uint64_t l3: 8;
		uint64_t l2: 8;
		uint64_t l1: 8;
		uint64_t flags: 8;
	} __attribute__ ((__packed__));
} __attribute__ ((__packed__));

static_assert(sizeof(handle) == sizeof(handle_type));

void init_object_manager(); // in object_manager.cc

class handle_table final
{
 public:
	friend class handle_entry;
	friend struct handle_entry_deleter;

	friend void object::init_object_manager();

	static constexpr size_t MAX_HANDLE_PER_TABLE = 512;

	struct table
	{
		size_t count;
		union
		{
			table* next[MAX_HANDLE_PER_TABLE];
			handle_entry* entry[MAX_HANDLE_PER_TABLE];
		} __attribute__ ((__packed__));
	} __attribute__ ((__packed__));

	handle_table();

	handle_table(dispatcher* parent);

	~handle_table() = default;

	handle_table(const handle_table&) = delete;
	handle_table(handle_table&&) = delete;
	handle_table& operator=(const handle_table&) = delete;

	template<std::derived_from<dispatcher> T>
	error_code_with_result<T*> object_from_handle(const handle_entry& h)
	{
		return downcast_dispatcher<T>(h.ptr_);
	}

	template<std::derived_from<dispatcher> T>
	error_code_with_result<T*> object_from_handle(handle_entry* h)
	{
		return downcast_dispatcher<T>(h->ptr_);
	}

	handle_type add_handle(handle_entry_owner owner);
	handle_type add_handle_locked(handle_entry_owner owner)TA_REQ(lock_);
	handle_type entry_to_handle(handle_entry* h) const;

	handle_entry_owner remove_handle(handle_type h);
	handle_entry_owner remove_handle(handle_entry* e);

	handle_entry_owner remove_handle_locked(handle_type h) TA_REQ(lock_);
	handle_entry_owner remove_handle_locked(handle_entry* e)TA_REQ(lock_);

	handle_entry* get_handle_entry(handle_type h);

	handle_entry* get_handle_entry_locked(handle_type h) TA_REQ(lock_);

	handle_entry* query_handle_by_name(ktl::string_view name);
	handle_entry* query_handle_by_name_locked(ktl::string_view name)  TA_REQ(lock_);

	template<typename T>
	handle_entry* query_handle(T&& pred);

	template<typename T>
	handle_entry* query_handle_locked(T&& pred)  TA_REQ(lock_);

	void clear();

 private:
	[[nodiscard]] static std::tuple<int, int> increase_next_cur(size_t value);
	[[nodiscard]] error_code increase_next();

	void initialize_table();

	error_code_with_result<std::tuple<size_t, size_t, size_t, size_t>> allocate_slot();

	error_code_with_result<std::tuple<size_t, size_t, size_t, size_t>> first_free();

	explicit handle_table(global_handle_table_tag);

	bool local_exist_locked(handle_entry* owner) TA_REQ(lock_);

	std::optional<std::tuple<size_t, size_t, size_t, size_t>> local_get_locked(handle_entry* owner) TA_REQ(lock_);

	static constexpr handle_type MAKE_HANDLE(uint16_t attr, size_t l1, size_t l2, size_t l3, size_t l4)
	{
		return ((uint64_t)attr) << 32u | ((uint64_t)l1) << 24 | ((uint64_t)l2) << 16 | ((uint64_t)l3) << 8
			| ((uint64_t)l4);
	}

	static constexpr auto DISASSEMBLE_HANDLE(handle_type h)
	{
		return std::make_tuple(h >> 32, (h >> 24) & 0xFF, (h >> 16) & 0xFF, (h >> 8) & 0xFF, h & 0xFF);
	}

	bool local_{ true };

	dispatcher* parent_{ nullptr };

	table root_{};

	memory::kmem::kmem_cache* table_cache_{ nullptr };

	struct
	{
		size_t l1, l2, l3, l4;
	} next_{ 0, 0, 0, 0 };

	mutable lock::spinlock lock_;

};

template<typename T>
handle_entry* handle_table::query_handle(T&& pred)
{
	lock::lock_guard g{ lock_ };
	return query_handle_locked(pred);
}

template<typename T>
handle_entry* handle_table::query_handle_locked(T&& pred) TA_REQ(lock_)
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
					if (slot && pred(*slot))
					{
						return slot;
					}
				}
			}
		}
	}

	return nullptr;
}

}

