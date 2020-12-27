#pragma once

#include "system/types.h"
#include "system/concepts.hpp"

#include <any>

namespace libkernel
{

	template<Pointer TPtr>
	class linked_list_base
	{
	 public:
		template<typename T>
		using pred_type = bool (*)(TPtr, T&& key);

	 private:
		TPtr next{ nullptr };
		TPtr prev{ nullptr };

	 public:
		linked_list_base()
			: next{ static_cast<TPtr>(this) }, prev{ static_cast<TPtr>(this) }
		{
		}

		void insert_after(linked_list_base* newnode)
		{
			newnode->next = this->next;
			this->next->prev = static_cast<TPtr>(newnode);

			newnode->prev = static_cast<TPtr>(this);
			this->next = static_cast<TPtr>(newnode);
		}

		void remove()
		{
			this->next->prev = this->prev;
			this->prev->next = this->next;

			this->prev = nullptr;
			this->next = nullptr;
		}

		template<typename T>
		TPtr find(pred_type<T> pred, T key)
		{
			for (auto iter = this; iter; iter = iter->next)
			{
				if (pred(iter, key))
				{
					return iter;
				}
			}
		}

	 public:

		[[nodiscard]]TPtr get_prev() const
		{
			return prev;
		}

		[[nodiscard]]TPtr get_next() const
		{
			return next;
		}

	};

#define llb_for(pos, head) \
    for ((pos) = (head)->get_next(); (pos) != (head); (pos) = (pos)->get_next())


#define llb_for_safe(pos, n, head) \
    for (pos = (head)->get_next(), n = pos->get_next(); pos != (head); pos = n, n = pos->get_next())

	template<Pointer TPtr>
	class single_linked_child_list_base
	{
	 public:
		using pred_type = bool (*)(TPtr p, const void* key);

	 private:
		TPtr next;
		TPtr first;

	 protected:

		error_code add_node(single_linked_child_list_base* child)
		{
			child->next = static_cast<TPtr>(this->first);
			this->first = static_cast<TPtr>(child);
			return ERROR_SUCCESS;
		}

		error_code remove_node(single_linked_child_list_base* child)
		{
			if (this->first == child)
			{
				this->first = child->next;
				child->next = NULL;
				return ERROR_SUCCESS;
			}

			for (auto node = this->first; node != nullptr; node = node->next)
			{
				if (node->next == child)
				{
					node->next = static_cast<TPtr>(child->next);
					child->next = nullptr;
					return ERROR_SUCCESS;
				}
			}
			return -ERROR_NO_ENTRY;
		}

		TPtr find_first(pred_type pred, const void* key)
		{
			return find_next(this->first, pred, key);
		}

		TPtr find_next(single_linked_child_list_base* from, pred_type pred, const void* key)
		{
			for (auto node = from; node != nullptr; node = node->next)
			{
				if (pred(static_cast<TPtr>(node), key))
				{
					return static_cast<TPtr>(node);
				}
			}

			return nullptr;
		}

	 public:

		[[nodiscard]]TPtr get_first() const
		{
			return first;
		}

		[[nodiscard]]TPtr get_next() const
		{
			return next;
		}

	};

}