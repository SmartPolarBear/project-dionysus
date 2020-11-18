#pragma once

#include "data/list.h"
#include "drivers/lock/spinlock.h"
#include "system/kmalloc.hpp"

namespace libkernel
{
	template<typename T>
	class stack
	{
	 private:
		struct node
		{
			T member;
			list_head link;
		};

		list_head head;
		lock::spinlock stack_lock;

		size_t m_size;
	 public:
		stack() : m_size(0)
		{
			list_init(&this->head);
			lock::spinlock_initialize_lock(&stack_lock, __PRETTY_FUNCTION__);
		}

		~stack()
		{
			clear();
		}

		void clear()
		{
			lock::spinlock_acquire(&stack_lock);

			list_head* iter = nullptr, * t = nullptr;
			list_for_safe(iter, t, &this->head)
			{
				node* n = list_entry(iter, node, link);
				list_remove(iter);
				delete n;
			}
			m_size = 0;
			lock::spinlock_release(&stack_lock);
		}

		void push(T data)
		{
			lock::spinlock_acquire(&stack_lock);

			node* new_node = new node;
			new_node->member = data;

			list_add(&new_node->link, &head);
			m_size++;
			lock::spinlock_release(&stack_lock);
		}

		const T& top()
		{
			node* n = list_entry(head.next, node, link);
			return n->member;
		}

		void pop()
		{
			lock::spinlock_acquire(&stack_lock);

			node* n = list_entry(head.next, node, link);

			list_remove(&n->link);
			delete n;
			m_size--;
			lock::spinlock_release(&stack_lock);

		}

		size_t size()
		{
			return m_size;
		}
	};
}
