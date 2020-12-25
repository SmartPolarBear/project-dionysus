#pragma once

#include "system/types.h"
#include "system/concepts.hpp"

namespace libkernel
{
	template<typename TPtr>
	requires Pointer<TPtr>
	class single_linked_list_base
	{
	 public:
		using pred_type = bool (*)(TPtr p, const void* key);

	 private:
		TPtr next;
		TPtr first;

	 protected:

		error_code add_node(single_linked_list_base* child)
		{
			child->next = static_cast<TPtr>(this->first);
			this->first = static_cast<TPtr>(child);
			return ERROR_SUCCESS;
		}

		error_code remove_node(single_linked_list_base* child)
		{
			if (this->first == child)
			{
				this->first = child->next;
				child->next = NULL;
				return ERROR_SUCCESS;
			}

			for (auto node = this->first; node != nullptr; node = node->head)
			{
				if (node->head == child)
				{
					node->head = static_cast<TPtr>(child->next);
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

		TPtr find_next(single_linked_list_base* from, pred_type pred, const void* key)
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