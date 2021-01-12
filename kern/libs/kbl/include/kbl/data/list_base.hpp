#pragma once

#include "system/types.h"
#include "system/error.hpp"

#include "ktl/concepts.hpp"

#include <any>
#include <concepts>

namespace kbl
{

template<typename TList, typename TChild>
concept ListOfTWithBound=
requires(TList l){ l[0]; (l[0])->TChild; };

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
		return (TPtr)
		this;
	}

	template<ktl::Pointer NewT>
	[[nodiscard]]NewT get_element_as() const
	{
		return (NewT)(TPtr)
		this;
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

template<typename T>
requires (!ktl::Pointer<T>)
class doubly_linked_node_state
{
	template<typename S, doubly_linked_node_state<S> S::*>
	friend
	class intrusive_doubly_linked_list;

	template<typename S, doubly_linked_node_state<S> S::*>
	friend
	class intrusive_doubly_linked_list_iterator;

	T* next{ nullptr };
	doubly_linked_node_state<T>* prev{ nullptr };

 public:
	doubly_linked_node_state() = default;

	doubly_linked_node_state(const doubly_linked_node_state&) = delete;

	void operator=(const doubly_linked_node_state&) = delete;
};

template<typename T, doubly_linked_node_state<T> T::*NS>
class intrusive_doubly_linked_list_iterator
{
 public:
	explicit intrusive_doubly_linked_list_iterator(doubly_linked_node_state<T>* ns)
		: node_state(ns)
	{
	}

	T& operator*()
	{
		return *this->operator->();
	}

	T* operator->()
	{
		return this->node_state->next;
	}

	bool operator==(intrusive_doubly_linked_list_iterator const& other) const
	{
		return this->node_state == other.node_state;
	}

	bool operator!=(intrusive_doubly_linked_list_iterator const& other) const
	{
		return !(*this == other);
	}

	intrusive_doubly_linked_list_iterator& operator++()
	{
		this->node_state = &(this->node_state->next->*NS);
		return *this;
	}

	intrusive_doubly_linked_list_iterator operator++(int)
	{
		intrusive_doubly_linked_list_iterator rc(*this);
		this->operator++();
		return rc;
	}

	intrusive_doubly_linked_list_iterator& operator--()
	{
		this->node_state = this->node_state->prev;
		return *this;
	}

	intrusive_doubly_linked_list_iterator operator--(int)
	{
		intrusive_doubly_linked_list_iterator rc(*this);
		this->operator--();
		return rc;
	}

 private:
	template<typename S, doubly_linked_node_state<S> S::*>
	friend
	class intrusive_doubly_linked_list;

	doubly_linked_node_state<T>* node_state;
};

template<typename T, doubly_linked_node_state<T> T::*NS>
class intrusive_doubly_linked_list
{
 public:
	intrusive_doubly_linked_list()
	{
		this->head.prev = &this->head;
	}

	intrusive_doubly_linked_list_iterator<T, NS> begin()
	{
		return intrusive_doubly_linked_list_iterator<T, NS>(&this->head);
	}

	intrusive_doubly_linked_list_iterator<T, NS> end()
	{
		return intrusive_doubly_linked_list_iterator<T, NS>(this->head.prev);
	}

	T& front()
	{
		return *this->head.next;
	}

	T& back()
	{
		return *(this->head.prev->prev->next);
	}

	bool empty() const
	{
		return &this->head == this->head.prev;
	}

	void push_back(T& node)
	{
		this->insert(this->end(), node);
	}

	void push_front(T& node)
	{
		this->insert(this->begin(), node);
	}

	void insert(intrusive_doubly_linked_list_iterator<T, NS> pos, T& node)
	{
		(node.*NS).next = pos.node_state->next;
		((node.*NS).next
		 ? (pos.node_state->next->*NS).prev
		 : this->head.prev) = &(node.*NS);
		(node.*NS).prev = pos.node_state;
		pos.node_state->next = &node;
	}

	intrusive_doubly_linked_list_iterator<T, NS> erase(intrusive_doubly_linked_list_iterator<T, NS> it)
	{
		it.node_state->next = (it.node_state->next->*NS).next;
		(it.node_state->next
		 ? (it.node_state->next->*NS).prev
		 : this->head.prev) = it.node_state;
		return it;
	}

 private:
	doubly_linked_node_state<T> head;
};

}
