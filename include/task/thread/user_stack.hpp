#pragma once

#include "system/types.h"

#include "kbl/data/list.hpp"
#include "kbl/lock/spinlock.h"

namespace task
{

class process;
class thread;

class user_stack
{
 public:
	friend class process;
	friend class thread;
	friend class process_user_stack_state;
	friend struct user_stack_list_node_trait;

	using link_type = kbl::list_link<user_stack, lock::spinlock>;

 public:

	user_stack() = delete;
	user_stack(const user_stack&) = delete;
	user_stack& operator=(const user_stack&) = delete;

	[[nodiscard]] void* get_top();

	int64_t operator<=>(const user_stack& rhs) const
	{
		return (uint8_t*)this->top - (uint8_t*)rhs.top;
	}

 private:
	user_stack(process* p, thread* t, void* stack_ptr);

	void* top{ nullptr };

	process* owner_process{ nullptr };
	thread* owner_thread{ nullptr };

	link_type process_owning_link_{ this };
};

struct user_stack_list_node_trait
{
	using reference_type = user_stack&;
	using pointer_type = user_stack*;
	using reference_return_type = user_stack::link_type&;
	using pointer_return_type = user_stack::link_type*;

	static reference_return_type node_link(reference_type element)
	{
		return element.process_owning_link_;
	}

	static reference_return_type node_link(pointer_type NONNULL element)
	{
		return element->process_owning_link_;
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

static_assert(kbl::NodeTrait<user_stack_list_node_trait, user_stack, lock::spinlock>);

}