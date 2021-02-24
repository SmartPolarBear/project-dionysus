#pragma once

#include "kbl/data/list.hpp"

#include "kbl/lock/spinlock.h"

#include "object/handle_entry.hpp"

namespace object
{

using handle_type = uint64_t;

class handle_table final
{
 public:
	static constexpr size_t MAX_HANDLE_PER_TABLE = UINT32_MAX;

	template<std::convertible_to<dispatcher> T>
	error_code_with_result<std::shared_ptr<T>> object_from_handle(const handle_entry& h);

	void add_handle(const handle_entry& h);
	void remove_handle(handle_entry& h);

	bool valid_handle(const handle_entry& h) const;

 private:
	kbl::intrusive_list<handle_entry,
	                    lock::spinlock,
	                    handle_entry::node_trait,
	                    true> handles_{};
};
}