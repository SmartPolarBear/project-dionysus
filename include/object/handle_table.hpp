#pragma once

#include "kbl/data/list.hpp"

#include "kbl/lock/spinlock.h"

#include "object/handle_entry.hpp"

namespace object
{

using handle_type = uint64_t;

enum [[clang::flag_enum]] handle_type_attributes
{
	HATTR_GLOBAL = 0b1,
	HATTR_LOCAL_PROC = 0b10,
};

class handle_table final
{
 public:
	friend class handle_entry;
	friend struct handle_entry_deleter;

	static constexpr size_t MAX_HANDLE_PER_TABLE = UINT32_MAX;

	template<std::convertible_to<dispatcher> T>
	error_code_with_result<std::shared_ptr<T>> object_from_handle(const handle_entry& h);

	void add_handle(handle_entry_owner owner);
	void add_handle_locked(handle_entry_owner owner);

	handle_entry_owner remove_handle(handle_type h);
	handle_entry_owner remove_handle_locked(handle_type h);
	handle_entry_owner remove_handle_locked(handle_entry* e);

	handle_entry* get_handle_entry(handle_type h);

 private:
	static constexpr uint32_t INDEX_OF_HANDLE(handle_type h)
	{
		return h & 0xFFFFFFFFu;
	}

	static constexpr uint32_t ATTR_OF_HANDLE(handle_type h)
	{
		return h >> 32u;
	}

	static constexpr auto PARSE_HANDLE(handle_type h)
	{
		return std::make_tuple(ATTR_OF_HANDLE(h), INDEX_OF_HANDLE(h));
	}

	kbl::intrusive_list<handle_entry,
	                    lock::spinlock,
	                    handle_entry::node_trait,
	                    true> handles_{};

	static kbl::intrusive_list<handle_entry,
	                           lock::spinlock,
	                           handle_entry::node_trait,
	                           true> global_handles_;

	mutable lock::spinlock lock_;
};
}