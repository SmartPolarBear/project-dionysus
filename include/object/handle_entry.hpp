#pragma once

#include "system/types.h"

#include "kbl/data/list.hpp"
#include "kbl/lock/spinlock.h"

#include "object/ref_counted.hpp"
#include "object/dispatcher.hpp"

#include "ktl/shared_ptr.hpp"

#include <any>
#include <utility>

namespace object
{

enum class [[clang::enum_extensibility(closed)]] handle_table_type
{
	LOCAL, GLOBAL
};

class handle_table;
class handle_entry;

struct handle_entry_deleter
{
	void operator()(handle_entry* e);
};

using handle_entry_owner = ktl::unique_ptr<handle_entry, handle_entry_deleter>;

class handle_entry final
{
 public:



	friend class handle_table;
	friend struct handle_entry_deleter;

	using link_type = kbl::list_link<handle_entry, lock::spinlock>;

	struct node_trait
	{
		using reference_type = handle_entry&;
		using pointer_type = handle_entry*;
		using reference_return_type = handle_entry::link_type&;
		using pointer_return_type = handle_entry::link_type*;

		static reference_return_type node_link(reference_type element)
		{
			return element.link_;
		}

		static reference_return_type node_link(pointer_type NONNULL element)
		{
			return element->link_;
		}

		static pointer_return_type NONNULL node_link_ptr(reference_type element)
		{
			return &node_link(element);
		}

		static pointer_return_type NONNULL node_link_ptr(pointer_type NONNULL element)
		{
			return &node_link(element);
		}
	};

	[[nodiscard]] static handle_entry_owner make(const dispatcher* obj, bool is_global);

	[[nodiscard]] static handle_entry_owner duplicate(handle_entry* h);

	static void release(handle_entry_owner h);
 private:
	handle_entry(const dispatcher* obj)
		: ptr_(const_cast<dispatcher*>(obj))
	{
	}

	kbl::canary<kbl::magic("HENT")> canary_;

	dispatcher* ptr_;
	handle_table* parent_;

	[[maybe_unused]] rights_type rights_{ 0 };

	koid_type owner_process_id{ -1 };

	link_type link_;

	bool operator==(const handle_entry& another) const
	{
		return ptr_ == another.ptr_;
	}

	bool operator!=(const handle_entry& another) const
	{
		return !operator==(another);
	}

};

}