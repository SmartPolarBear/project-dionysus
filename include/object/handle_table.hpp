#pragma once

#include "kbl/data/list.hpp"
#include "kbl/lock/spinlock.h"

#include "ktl/concepts.hpp"

#include "object/handle_entry.hpp"

namespace object
{

using handle_type = uint64_t;

enum [[clang::flag_enum]] handle_type_attributes : uint16_t
{
	HATTR_GLOBAL = 0b1,
	HATTR_LOCAL_PROC = 0b10,
};

struct global_handle_table_tag
{
};

static inline constexpr global_handle_table_tag create_global_handle_table;

class handle_table final
{
 public:
	friend class handle_entry;
	friend struct handle_entry_deleter;

	static constexpr size_t MAX_HANDLE_PER_TABLE = UINT32_MAX;

	static handle_table* get_global_handle_table();

	handle_table() : local_{ true }, parent_{ nullptr }
	{
	}

	handle_table(dispatcher* parent) : local_{ true }, parent_{ parent }
	{
	}

	~handle_table() = default;

	handle_table(const handle_table&) = delete;
	handle_table(handle_table&&) = delete;
	handle_table& operator=(const handle_table&) = delete;

	template<std::convertible_to<dispatcher> T>
	error_code_with_result<std::shared_ptr<T>> object_from_handle(const handle_entry& h);

	handle_type add_handle(handle_entry_owner owner);
	handle_type add_handle_locked(handle_entry_owner owner)TA_REQ(lock_);

	handle_entry_owner remove_handle(handle_type h);
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
	explicit handle_table(global_handle_table_tag) : local_{ false }, parent_{ nullptr }
	{
	}

	bool local_exist_locked(handle_entry* owner) TA_REQ(lock_);

	static constexpr uint64_t INDEX_OF_HANDLE(handle_type h)
	{
		return h & 0xFFFF'FFFF'FFFFull;
	}

	static constexpr uintptr_t INDEX_TO_ADDR(uint64_t idx)
	{
		return idx | 0xFFFF'0000'0000'0000ull;
	}

	static constexpr uint16_t ATTR_OF_HANDLE(handle_type h)
	{
		return h >> 48u;
	}

	static constexpr auto PARSE_HANDLE(handle_type h)
	{
		return std::make_tuple(ATTR_OF_HANDLE(h), INDEX_OF_HANDLE(h));
	}

	static constexpr handle_type MAKE_HANDLE(uint16_t attr, uintptr_t addr)
	{
		addr &= 0x0000'FFFF'FFFF'FFFFull;
		return ((uint64_t)attr << 48ull) | addr;
	}

	bool local_{ true };

	dispatcher* parent_{ nullptr };

	kbl::intrusive_list<handle_entry,
	                    lock::spinlock,
	                    handle_entry::node_trait,
	                    true> handles_{};

	mutable lock::spinlock lock_;

	static handle_table global_handle_table_;
};

}