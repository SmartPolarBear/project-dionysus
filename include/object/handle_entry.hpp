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

struct handle_entry_deleter
{
	void operator()(class handle_entry* e);
};

using handle_entry_owner = ktl::unique_ptr<class handle_entry, handle_entry_deleter>;

class handle_entry final
{
 public:
	friend class handle_table;

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

	[[nodiscard]] static handle_entry_owner make(const ktl::shared_ptr<dispatcher>& obj);

	[[nodiscard]] static handle_entry_owner duplicate(handle_entry* h);

	static void release(handle_entry_owner h);
 private:
	handle_entry(ktl::shared_ptr<dispatcher> obj)
		: ptr_(std::move(obj))
	{
	}

	kbl::canary<kbl::magic("HENT")> canary_;

	ktl::shared_ptr<dispatcher> ptr_;
	[[maybe_unused]] rights_type rights_{ 0 };

	[[maybe_unused]] std::weak_ptr<handle_table> parent_;
	[[maybe_unused]] koid_type owner_process_id;

	link_type link_;

};

}