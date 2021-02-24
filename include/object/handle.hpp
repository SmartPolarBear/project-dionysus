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

class handle final
{
 public:
	friend class handle_table;

	using link_type = kbl::list_link<handle, lock::spinlock>;

	struct node_trait
	{
		using reference_type = handle&;
		using pointer_type = handle*;
		using reference_return_type = handle::link_type&;
		using pointer_return_type = handle::link_type*;

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

	[[nodiscard]] static handle make(ktl::shared_ptr<dispatcher> obj, object_type t);

	[[nodiscard]] static handle duplicate(const handle& h);

	static void release(const handle& h);
 private:
	handle(ktl::shared_ptr<dispatcher> obj, object_type t)
		: obj_(std::move(obj)),
		  type_(t)
	{
	}

	ktl::shared_ptr<dispatcher> obj_;
	[[maybe_unused]] object_type type_;
	[[maybe_unused]] handle_table_type table_{ handle_table_type::GLOBAL };
	[[maybe_unused]] rights_type rights_{ 0 };

	link_type link_;
};

}