#pragma once

#include "data/List.h"
#include "drivers/lock/spinlock.h"
#include "system/kmalloc.hpp"

namespace libkernel
{
	template<typename T>
	class Queue
	{
	 private:
		struct node
		{
			T member;
			list_head link;
		};

		list_head head;
		lock::spinlock queue_lock;

		size_t m_size;
	 public:
		Queue() : m_size(0)
		{
			list_init(&this->head);
			lock::spinlock_initlock(&queue_lock, __PRETTY_FUNCTION__);
		}

		~Queue()
		{
			clear();
		}

		void clear()
		{
			lock::spinlock_acquire(&queue_lock);

			list_head* iter = nullptr, * t = nullptr;
			list_for_safe(iter, t, &this->head)
			{
				node* n = list_entry(iter, node, link);
				list_remove(iter);
				delete n;
			}
			m_size=0;
			lock::spinlock_release(&queue_lock);
		}

		void push(T data)
		{
			lock::spinlock_acquire(&queue_lock);

			node* new_node = new node;
			new_node->member = data;

			list_add(&new_node->link, &head);
			m_size++;
			lock::spinlock_release(&queue_lock);
		}

		T pop()
		{
			lock::spinlock_acquire(&queue_lock);

			node* n = list_entry(head.prev, node, link);

			T data = n->member;
			list_remove(&n->link);
			delete n;
			m_size--;
			lock::spinlock_release(&queue_lock);

			return data;
		}

		size_t size()
		{
			return m_size;
		}
	};
}
