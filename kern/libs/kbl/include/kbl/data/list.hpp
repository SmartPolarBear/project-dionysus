#pragma once

#include "ktl/mutex/lock_guard.hpp"

namespace kbl
{
// linked list head_
template<typename TParent, typename TMutex>
struct list_link
{
	TParent* parent;
	bool is_head;
	list_link* next, * prev;

	mutable TMutex lock;

	list_link() : parent{ nullptr }, is_head{ false }, next{ this }, prev{ this }
	{
	}

	list_link(list_link&& another) noexcept
	{
		parent = another.parent;
		is_head = another.is_head;
		next = another.next;
		prev = another.prev;

		another.parent = nullptr;
		another.is_head = false;
		another.next = &another;
		another.prev = &another;
	}

	list_link(const list_link& another)
	{
		parent = another.parent;
		is_head = another.is_head;
		next = another.next;
		prev = another.prev;
	}

	list_link& operator=(const list_link& another)
	{
		if (this == &another)return *this;

		parent = another.parent;
		is_head = another.is_head;
		next = another.next;
		prev = another.prev;
	}

	explicit list_link(TParent* p) : parent{ p }, is_head{ false }, next{ this }, prev{ this }
	{
	}

	explicit list_link(TParent& p) : parent{ &p }, is_head{ false }, next{ this }, prev{ this }
	{
	}


	bool operator==(const list_link& that) const
	{
		return parent == that.parent &&
			is_head == that.is_head &&
			next == that.next &&
			prev == that.prev;
	}

	bool operator!=(const list_link& that) const
	{
		return !(*this == that);
	}
};

template<typename T, typename TMutex, bool Reverse = false, bool EnableLock = false>
class intrusive_list_iterator
{
 public:

	template<typename S, typename SMutex, list_link<S, SMutex> S::*, bool E, bool D>
	friend
	class intrusive_list;

	using value_type = T;
	using mutex_type = TMutex;
	using head_type = list_link<value_type, mutex_type>;
	using size_type = size_t;
	using dummy_type = int;

 public:
	intrusive_list_iterator() = default;

	explicit intrusive_list_iterator(head_type* h) : h_(h)
	{
	}

	intrusive_list_iterator(intrusive_list_iterator&& another) noexcept
	{
		h_ = another.h_;
		another.h_ = nullptr;
	}

	intrusive_list_iterator(const intrusive_list_iterator& another)
	{
		h_ = another.h_;
	}

	intrusive_list_iterator& operator=(const intrusive_list_iterator& another)
	{
		if (this == &another)return *this;

		h_ = another.h_;
		return *this;
	}


	T& operator*()
	{
		return *operator->();
	}

	T* operator->()
	{
		return h_->parent;
	}

	bool operator==(intrusive_list_iterator const& other) const
	{
		return h_ == other.h_;
	}

	bool operator!=(intrusive_list_iterator const& other) const
	{
		return !(*this == other);
	}

	intrusive_list_iterator& operator++()
	{
		if constexpr (Reverse)
		{
			h_ = h_->prev;
		}
		else
		{
			h_ = h_->next;
		}
		return *this;
	}

	intrusive_list_iterator operator++(dummy_type) noexcept
	{
		intrusive_list_iterator rc(*this);
		operator++();
		return rc;
	}

	intrusive_list_iterator& operator--()
	{
		if constexpr (Reverse)
		{
			h_ = h_->next;
		}
		else
		{
			h_ = h_->prev;
		}
		return *this;
	}

	intrusive_list_iterator operator--(dummy_type) noexcept
	{
		intrusive_list_iterator rc(*this);
		operator--();
		return rc;
	}

 private:
	head_type* h_;
	mutable mutex_type lock;
};

#pragma push_macro("list_for")
#pragma push_macro("list_for_safe")

#ifdef list_for
#undef list_for
#endif

#ifdef list_for_safe
#undef list_for_safe
#endif

#define list_for(pos, head) \
    for ((pos) = (head)->next; (pos) != (head); (pos) = (pos)->next)

#define list_for_safe(pos, n, head) \
    for (pos = (head)->next, n = pos->next; pos != (head); pos = n, n = pos->next)

template<typename T,
	typename TMutex,
	list_link<T, TMutex> T::*Link,
	bool EnableLock = false,
	bool CallDeleteOnRemoval = false>
class intrusive_list
{
 public:
	using value_type = T;
	using mutex_type = TMutex;
	using head_type = list_link<value_type, mutex_type>;
	using size_type = size_t;
	using container_type = intrusive_list<T, TMutex, Link, EnableLock, CallDeleteOnRemoval>;
	using iterator_type = intrusive_list_iterator<T, TMutex, false, EnableLock>;
	using riterator_type = intrusive_list_iterator<T, TMutex, true, EnableLock>;
	using const_iterator_type = const iterator_type;

	using lock_guard_type = ktl::mutex::lock_guard<TMutex>;
 public:
	/// New empty list
	constexpr intrusive_list()
	{
		if constexpr (EnableLock)
		{
			lock_guard_type g{ lock };
			list_init(&head_);
		}
		else
		{
			list_init(&head_);
		}
	}

	/// Isn't copiable
	intrusive_list(const intrusive_list&) = delete;

	intrusive_list& operator=(const intrusive_list&) = delete;

	/// Move constructor
	intrusive_list(intrusive_list&& another) noexcept
	{
		if constexpr (EnableLock)
		{
			lock_guard_type g{ lock };
			list_init(&head_);

			head_type* iter = nullptr;
			head_type* t = nullptr;

			list_for_safe(iter, t, &another.head_)
			{
				list_remove(iter);
				list_add(iter, this->head_);
			}

			size_ = another.size_;
			another.size_ = 0;
		}
		else
		{
			list_init(&head_);

			head_type* iter = nullptr;
			head_type* t = nullptr;

			list_for_safe(iter, t, &another.head_)
			{
				list_remove(iter);
				list_add(iter, this->head_);
			}

			size_ = another.size_;
			another.size_ = 0;
		}
	}

	/// First element
	/// \return the reference to first element
	T& front()
	{
		return *head_.next->parent;
	}

	T* front_ptr()
	{
		return head_.next->parent;
	}

	/// Last element
	/// \return the reference to first element
	T& back()
	{
		return *head_.prev->parent;
	}

	T* back_ptr()
	{
		return head_.prev->parent;
	}

	iterator_type begin()
	{
		return iterator_type{ head_.next };
	}

	iterator_type end()
	{
		return iterator_type{ &head_ };
	}

	const_iterator_type cbegin()
	{
		return const_iterator_type{ head_.next };
	}

	const_iterator_type cend()
	{
		return const_iterator_type{ &head_ };
	}

	riterator_type rbegin()
	{
		return riterator_type{ head_.prev };
	}

	riterator_type rend()
	{
		return riterator_type{ &head_ };
	}

	void insert(const_iterator_type iter, T& item)
	{
		if constexpr (EnableLock)
		{
			lock_guard_type g{ lock };
			list_add(&item.*Link, iter.h_);
			++size_;
		}
		else
		{
			list_add(&item.*Link, iter.h_);
			++size_;
		}

	}

	void insert(const_iterator_type iter, T* item)
	{
		if constexpr (EnableLock)
		{
			lock_guard_type g{ lock };
			list_add(&(item->*Link), iter.h_);
			++size_;
		}
		else
		{
			list_add(&(item->*Link), iter.h_);
			++size_;
		}


	}

	void insert(riterator_type iter, T& item)
	{
		if constexpr (EnableLock)
		{
			lock_guard_type g{ lock };
			list_add(&item.*Link, iter.h_);
			++size_;
		}
		else
		{
			list_add(&item.*Link, iter.h_);
			++size_;
		}
	}

	void insert(riterator_type iter, T* item)
	{
		if constexpr (EnableLock)
		{
			lock_guard_type g{ lock };
			list_add(&(item->*Link), iter.h_);
			++size_;

		}
		else
		{
			list_add(&(item->*Link), iter.h_);
			++size_;
		}
	}

	void erase(iterator_type it, bool call_delete = CallDeleteOnRemoval)
	{
		if (list_empty(&head_))return;

		if constexpr (EnableLock)
		{
			lock_guard_type g{ lock };

			list_remove(it.h_);
			if (call_delete)delete it.h_->parent;
			--size_;
		}
		else
		{
			list_remove(it.h_);
			if (call_delete)delete it.h_->parent;
			--size_;
		}
	}

	void erase(riterator_type it, bool call_delete = CallDeleteOnRemoval)
	{
		if (list_empty(&head_))return;

		if constexpr (EnableLock)
		{
			lock_guard_type g{ lock };

			list_remove(it.h_);
			if (call_delete)delete it.h_->parent;
			--size_;
		}
		else
		{
			list_remove(it.h_);
			if (call_delete)delete it.h_->parent;
			--size_;
		}
	}

	/// Remove item by value. **it takes liner time**
	/// \param val
	void remove(T& val, bool call_delete = CallDeleteOnRemoval)
	{
		if (list_empty(&head_))return;


		if constexpr (EnableLock)
		{
			lock_guard_type g{ lock };
			list_remove(&(val.*Link));
			if (call_delete)delete &val;
			--size_;
		}
		else
		{
			list_remove(&(val.*Link));
			if (call_delete)delete &val;
			--size_;
		}

	}

	void pop_back(bool call_delete = CallDeleteOnRemoval)
	{
		if (list_empty(&head_))return;

		if constexpr (EnableLock)
		{
			lock_guard_type g{ lock };
			auto entry = head_.prev;
			list_remove(entry);
			--size_;

			if (call_delete)delete entry->parent;
		}
		else
		{
			list_remove(head_.prev);
			auto entry = head_.prev;
			list_remove(entry);
			--size_;

			if (call_delete)delete entry->parent;
		}

	}

	void push_back(T& item)
	{
		if constexpr (EnableLock)
		{
			lock_guard_type g{ lock };
			list_add_tail(&item.*Link, &head_);
			++size_;
		}
		else
		{
			list_add_tail(&item.*Link, &head_);
			++size_;
		}

	}

	void push_back(T* item)
	{
		if constexpr (EnableLock)
		{
			lock_guard_type g{ lock };
			list_add_tail(&(item->*Link), &head_);
			++size_;
		}
		else
		{
			list_add_tail(&(item->*Link), &head_);
			++size_;
		}

	}

	void pop_front(bool call_delete = CallDeleteOnRemoval)
	{
		if (list_empty(&head_))return;

		if constexpr (EnableLock)
		{
			lock_guard_type g{ lock };
			auto entry = head_.next;
			list_remove(entry);
			--size_;

			if (call_delete)delete entry->parent;
		}
		else
		{
			auto entry = head_.next;
			list_remove(entry);
			--size_;

			if (call_delete)delete entry->parent;
		}

	}

	void push_front(T& item)
	{
		if constexpr (EnableLock)
		{
			lock_guard_type g{ lock };
			list_add(&item.*Link, &head_);
			++size_;
		}
		else
		{
			list_add(&item.*Link, &head_);
			++size_;
		}

	}

	void push_front(T* item)
	{
		if constexpr (EnableLock)
		{
			lock_guard_type g{ lock };
			list_add(&(item->*Link), &head_);
			++size_;
		}
		else
		{
			list_add(&(item->*Link), &head_);
			++size_;
		}

	}

	void clear(bool call_delete = CallDeleteOnRemoval)
	{
		if constexpr (EnableLock)
		{
			lock_guard_type g{ lock };
			do_clear(call_delete);
		}
		else
		{
			do_clear(call_delete);
		}
	}

	/// swap this and another
	/// \param another
	void swap(container_type& another) noexcept
	{
		if constexpr (EnableLock)
		{
			lock_guard_type g{ lock };
			list_swap(&head_, &another.head_);

			size_type t = size_;
			size_ = another.size_;
			another.size_ = t;
		}
		else
		{
			list_swap(&head_, &another.head_);

			size_type t = size_;
			size_ = another.size_;
			another.size_ = t;
		}

	}

	/// Merge two **sorted** list, after that another becomes empty
	/// \param another
	void merge(container_type& another)
	{
		merge(another, [](const T& a, const T& b)
		{
		  return a < b;
		});
	}

	/// Merge two **sorted** list, after that another becomes empty
	/// \tparam Compare cmp(a,b) returns true if a comes before b
	/// \param another
	/// \param cmp
	template<typename Compare>
	void merge(container_type& another, Compare cmp)
	{
		if constexpr (EnableLock)
		{
			lock_guard_type g1{ lock };
			lock_guard_type g2{ another.lock };

			do_merge(another, cmp);
		}
		else
		{
			do_merge(another, cmp);
		}
	}

	/// join two lists
	/// \param other
	void splice(intrusive_list& other)
	{
		if constexpr (EnableLock)
		{
			lock_guard_type g{ lock };

			size_ += other.size_;
			other.size_ = 0;

			list_splice_init(&other.head_, &head_);
		}
		else
		{
			size_ += other.size_;
			other.size_ = 0;

			list_splice_init(&other.head_, &head_);
		}

	}


	/// Join two lists, insert other 's item after the pos
	/// \param pos insert after it
	/// \param other
	void splice(const_iterator_type pos, intrusive_list& other)
	{
		if constexpr (EnableLock)
		{
			lock_guard_type g{ lock };

			size_ += other.size_;
			other.size_ = 0;

			list_splice_init(&other.head_, pos.h_);

		}
		else
		{
			size_ += other.size_;
			other.size_ = 0;

			list_splice_init(&other.head_, pos.h_);
		}

	}

	/// Join two lists, insert other 's item after the pos
	/// \param pos insert after it
	/// \param other
	void splice(riterator_type pos, intrusive_list& other)
	{
		if constexpr (EnableLock)
		{
			lock_guard_type g{ lock };

			size_ += other.size_;
			other.size_ = 0;

			list_splice_init(&other.head_, pos.h_);
		}
		else
		{
			size_ += other.size_;
			other.size_ = 0;

			list_splice_init(&other.head_, pos.h_);
		}

	}

	[[nodiscard]] size_type size() const
	{
		return size_;
	}

	[[nodiscard]] size_type size_slow() const
	{
		if constexpr (EnableLock)
		{
			lock_guard_type g{ lock };
			return do_size_slow();
		}
		else
		{
			return do_size_slow();
		}

	}

	[[nodiscard]] bool empty() const
	{
		return list_empty(&head_);
	}

 private:


	void do_clear(bool call_delete = false)
	{
		if (!list_empty(&head_))
		{
			head_type* iter = nullptr, * t = nullptr;
			list_for_safe(iter, t, &head_)
			{
				list_remove(iter);
				if (call_delete && iter->parent)
				{
					delete iter->parent;
				}
			}
		}
		size_ = 0;
	}

	size_type do_size_slow() const
	{
		size_type sz = 0;
		head_type* iter = nullptr;
		list_for(iter, &head_)
		{
			sz++;
		}
		return sz;
	}

	template<typename Compare>
	void do_merge(container_type& another, Compare cmp)
	{
		if (head_ == another.head_)return;

		size_ += another.size_;

		head_type t_head{ nullptr };
		head_type* i1 = head_.next, * i2 = another.head_.next;
		while (i1 != &head_ && i2 != &another.head_)
		{
			if (cmp(*(i1->parent), *(i2->parent)))
			{
				auto next = i1->next;
				list_remove(i1);
				list_add_tail(i1, &t_head);
				i1 = next;
			}
			else
			{
				auto next = i2->next;
				list_remove(i2);
				list_add_tail(i2, &t_head);
				i2 = next;
			}
		}

		while (i1 != &head_)
		{
			auto next = i1->next;
			list_remove_init(i1);
			list_add_tail(i1, &t_head);
			i1 = next;
		}

		while (i2 != &another.head_)
		{
			auto next = i2->next;
			list_remove_init(i2);
			list_add_tail(i2, &t_head);
			i2 = next;
		}

		another.size_ = 0;
		list_swap(&head_, &t_head);
	}

 private:
	static inline void util_list_init(head_type* head)
	{
		head->next = head;
		head->prev = head;
	}

	static inline void
	util_list_add(head_type* newnode, head_type* prev, head_type* next)
	{
		prev->next = newnode;
		next->prev = newnode;

		newnode->next = next;
		newnode->prev = prev;
	}

	static inline void util_list_remove(head_type* prev, head_type* next)
	{
		prev->next = next;
		next->prev = prev;
	}

	static inline void util_list_remove_entry(head_type* entry)
	{
		util_list_remove(entry->prev, entry->next);
	}

	static inline void
	util_list_splice(const head_type* list, head_type* prev, head_type* next)
	{
		head_type* first = list->next, * last = list->prev;

		first->prev = prev;
		prev->next = first;

		last->next = next;
		next->prev = last;
	}

 private:

	// initialize the list
	template<bool LockHeld = false>
	static inline void list_init(head_type* head)
	{
		if constexpr (EnableLock && !LockHeld)
		{
			lock_guard_type g{ head->lock };
			util_list_init(head);
		}
		else
		{
			util_list_init(head);
		}
	}

// add the new node after the specified head_
	template<bool LockHeld = false>
	static inline void list_add(head_type* newnode, head_type* head)
	{
		if constexpr (EnableLock && !LockHeld)
		{
			lock_guard_type g1{ newnode->lock };
			lock_guard_type g2{ head->lock };

			util_list_add(newnode, head, head->next);
		}
		else
		{
			util_list_add(newnode, head, head->next);
		}
	}

// add the new node before the specified head_
	template<bool LockHeld = false>
	static inline void list_add_tail(head_type* newnode, head_type* head)
	{
		if constexpr (EnableLock && !LockHeld)
		{
			lock_guard_type g1{ newnode->lock };
			lock_guard_type g2{ head->lock };

			util_list_add(newnode, head->prev, head);

		}
		else
		{
			util_list_add(newnode, head->prev, head);

		}
	}

// delete the entry
	template<bool LockHeld = false>
	static inline void list_remove(head_type* entry)
	{
		if constexpr (EnableLock && !LockHeld)
		{
			lock_guard_type g1{ entry->lock };

			util_list_remove_entry(entry);

			entry->prev = nullptr;
			entry->next = nullptr;
		}
		else
		{
			util_list_remove_entry(entry);

			entry->prev = nullptr;
			entry->next = nullptr;
		}
	}

	template<bool LockHeld = false>
	static inline void list_remove_init(head_type* entry)
	{
		if constexpr (EnableLock && !LockHeld)
		{
			lock_guard_type g1{ entry->lock };

			util_list_remove_entry(entry);

			entry->prev = nullptr;
			entry->next = nullptr;

			util_list_init(entry);
		}
		else
		{
			util_list_remove_entry(entry);

			entry->prev = nullptr;
			entry->next = nullptr;

			util_list_init(entry);
		}
	}

// replace the old entry with newnode
	template<bool LockHeld = false>
	static inline void list_replace(head_type* old, head_type* newnode)
	{
		if constexpr (EnableLock && !LockHeld)
		{
			newnode->next = old->next;
			newnode->next->prev = newnode;
			newnode->prev = old->prev;
			newnode->prev->next = newnode;
		}
		else
		{
			newnode->next = old->next;
			newnode->next->prev = newnode;
			newnode->prev = old->prev;
			newnode->prev->next = newnode;
		}

	}

	template<bool LockHeld = false>
	static inline bool list_empty(const head_type* head)
	{
		if constexpr (EnableLock && !LockHeld)
		{
			lock_guard_type g{ head->lock };

			return (head->next) == head;
		}
		else
		{
			return (head->next) == head;
		}
	}

	template<bool LockHeld = false>
	static inline void list_swap(head_type* e1, head_type* e2)
	{
		if constexpr (EnableLock && !LockHeld)
		{
			lock_guard_type g1{ e1->lock };
			lock_guard_type g2{ e2->lock };

			auto* pos = e2->prev;
			list_remove_init<true>(e2);
			list_replace<true>(e1, e2);

			if (pos == e1)
			{
				pos = e2;
			}

			list_add<true>(e1, pos);
		}
		else
		{
			auto* pos = e2->prev;
			list_remove_init(e2);
			list_replace(e1, e2);

			if (pos == e1)
			{
				pos = e2;
			}

			list_add(e1, pos);
		}
	}

	/// \tparam TParent
	/// \param list	the new list to add
	/// \param head the place to add it in the list
	static inline void list_splice(head_type* list, head_type* head)
	{
		if constexpr (EnableLock)
		{
			lock_guard_type g1{ list->lock };
			lock_guard_type g2{ head->lock };

			if (!list_empty<true>(list))
			{
				util_list_splice(list, head, head->next);
			}
		}
		else
		{
			if (!list_empty(list))
			{
				util_list_splice(list, head, head->next);
			}
		}
	}

	/// \tparam TParent
	/// \param list	the new list to add
	/// \param head the place to add it in the list
	static inline void list_splice_init(head_type* list, head_type* head)
	{
		if constexpr (EnableLock)
		{
			lock_guard_type g1{ list->lock };
			lock_guard_type g2{ head->lock };

			if (!list_empty<true>(list))
			{
				util_list_splice(list, head, head->next);
				util_list_init(list);
			}
		}
		else
		{
			if (!list_empty(list))
			{
				util_list_splice(list, head, head->next);
				util_list_init(list);
			}
		}
	}


	head_type head_{ nullptr };
	size_type size_{ 0 };

	mutable mutex_type lock;
};


#undef list_for
#undef list_for_safe

#pragma pop_macro("list_for")
#pragma pop_macro("list_for_safe")
} // namespace

