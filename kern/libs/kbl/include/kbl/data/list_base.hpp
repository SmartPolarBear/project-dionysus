#pragma once

#include "system/types.h"
#include "ktl/concepts.hpp"

#include <any>
#include <concepts>

namespace kbl
{

	template<ktl::Pointer TPtr>
	class linked_list_base
	{
	 public:
		template<typename T>
		using pred_type = bool (*)(TPtr, T&& key);
		using head_type = linked_list_base;
		using size_type = size_t;

	 private:
		TPtr next{ nullptr };
		TPtr prev{ nullptr };
		size_type _list_size{ 0 };

	 public:
		linked_list_base()
			: next{ static_cast<TPtr>(this) }, prev{ static_cast<TPtr>(this) }, _list_size{ 0 }
		{
		}

		void insert_after(linked_list_base* newnode)
		{
			newnode->next = this->next;
			this->next->prev = static_cast<TPtr>(newnode);

			newnode->prev = static_cast<TPtr>(this);
			this->next = static_cast<TPtr>(newnode);

			_list_size++;
		}

		void remove()
		{
			this->next->prev = this->prev;
			this->prev->next = this->next;

			this->prev = nullptr;
			this->next = nullptr;

			_list_size--;

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
			return nullptr;
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

		[[nodiscard]]TPtr get_element() const
		{
			return (TPtr)this;
		}

		template<ktl::Pointer NewT>
		[[nodiscard]]NewT get_element_as() const
		{
			return (NewT)(TPtr)this;
		}

		[[nodiscard]]bool empty() const
		{
			return !_list_size;
		}

		[[nodiscard]] size_type size() const
		{
			return _list_size;
		}
	};

#define llb_for(pos, head) \
    for ((pos) = (head)->get_next(); (pos) != (head); (pos) = (pos)->get_next())

#define llb_for_safe(pos, n, head) \
    for (pos = (head)->get_next(), n = pos->get_next(); pos != (head); pos = n, n = pos->get_next())

	template<ktl::Pointer TPtr>
	class single_linked_child_list_base
	{
	 public:
		using pred_type = bool (*)(TPtr p, const void* key);
		using size_type = size_t;

		error_code add_node(single_linked_child_list_base* child)
		{
			child->next = static_cast<TPtr>(this->first);
			this->first = static_cast<TPtr>(child);
			_list_size++;
			return ERROR_SUCCESS;
		}

		error_code remove_node(single_linked_child_list_base* child)
		{
			if (this->first == child)
			{
				this->first = child->next;
				child->next = NULL;
				_list_size--;
				return ERROR_SUCCESS;
			}

			for (auto node = this->first; node != nullptr; node = node->next)
			{
				if (node->next == child)
				{
					node->next = static_cast<TPtr>(child->next);
					child->next = nullptr;
					_list_size--;
					return ERROR_SUCCESS;
				}
			}
			return -ERROR_NO_ENTRY;
		}

		TPtr find_first(pred_type pred, const void* key)
		{
			return find_next(this->first, pred, key);
		}

		TPtr find_next(TPtr from, pred_type pred, const void* key)
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

		[[nodiscard]] bool empty() const
		{
			return !_list_size;
		}

		[[nodiscard]] size_type size() const
		{
			return _list_size;
		}

	 private:
		TPtr next;
		TPtr first;
		size_type _list_size{ 0 };
	};

}