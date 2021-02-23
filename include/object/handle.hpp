#pragma once

#include "system/types.h"

#include "kbl/data/list.hpp"
#include "kbl/lock/spinlock.h"

#include <any>

namespace object
{

enum class [[clang::enum_extensibility(closed)]] object_type
{
	PROCESS = 1,
	JOB,
	THREAD,
};

enum class [[clang::enum_extensibility(closed)]] handle_table_type
{
	LOCAL, GLOBAL
};

class handle final
{
 public:
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

	[[nodiscard]] static handle make(std::move_constructible auto& obj);

	[[nodiscard]] static handle duplicate(const handle& h);

	[[nodiscard]] static handle release(const handle& h);
 private:
	std::any obj_;
	object_type type_;
	handle_table_type table_;
	rights_type rights_;

	link_type link_;
};

}