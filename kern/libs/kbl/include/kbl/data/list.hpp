#pragma once
#include "debug/nullability.hpp"

#include "kbl/lock/lock_guard.hpp"

#include "kbl/data/utility.hpp"

#include "ktl/concepts.hpp"

#pragma push_macro("KDEBUG_ASSERT")

//#define ENABLE_DEBUG_MACRO
#ifndef ENABLE_DEBUG_MACRO
#ifdef KDEBUG_ASSERT
#undef KDEBUG_ASSERT
#endif
#define KDEBUG_ASSERT(...)
#endif

namespace kbl
{

template<typename T, typename E>
concept Deleter =
requires(T del, E* ptr)
{
	del(ptr);
};

// linked list head_
template<typename TParent, typename TMutex>
struct list_link
{
	TParent* NULLABLE parent;
	bool is_head;
	list_link* NONNULL next, * NONNULL prev;

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
		if (this == &another)
		{
			return *this;
		}

		parent = another.parent;
		is_head = another.is_head;
		next = another.next;
		prev = another.prev;
	}

	explicit list_link(TParent* NULLABLE p) : parent{ p }, is_head{ false }, next{ this }, prev{ this }
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

template<typename T, typename U, typename Mutex>
concept NodeTrait =
requires(U& u)
{
	{ T::node_link(u) }->ktl::convertible_to<list_link<U, Mutex>&>;
	{ T::node_link(&u) }->ktl::convertible_to<list_link<U, Mutex>&>;
	{ T::node_link_ptr(u) }->ktl::convertible_to<list_link<U, Mutex>*>;
	{ T::node_link_ptr(&u) }->ktl::convertible_to<list_link<U, Mutex>*>;
};

template<typename T, typename TMutex, class Container, bool EnableLock = false>
class intrusive_list_iterator
{
 public:
	friend Container;

	using value_type = T;
	using mutex_type = TMutex;
	using head_type = list_link<value_type, mutex_type>;
	using size_type = size_t;
	using dummy_type = int;

 public:
	constexpr intrusive_list_iterator() = default;

	constexpr explicit intrusive_list_iterator(head_type* NONNULL h) : h_(h)
	{
	}

	constexpr intrusive_list_iterator(intrusive_list_iterator&& another) noexcept
	{
		h_ = another.h_;
		another.h_ = nullptr;
	}

	constexpr intrusive_list_iterator(const intrusive_list_iterator& another)
	{
		h_ = another.h_;
	}

	intrusive_list_iterator& operator=(const intrusive_list_iterator& another) TA_NO_THREAD_SAFETY_ANALYSIS
	{
		if (this == &another)
		{
			return *this;
		}

		if constexpr(EnableLock)
		{
			lock_guard_type g{ lock_ };
			h_ = another.h_;
		}
		else
		{
			h_ = another.h_;

		}

		return *this;
	}

	T& operator*()
	{
		return *operator->();
	}

	T* NULLABLE operator->()
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

	intrusive_list_iterator& operator++() TA_NO_THREAD_SAFETY_ANALYSIS
	{
		if constexpr (EnableLock)
		{
			lock_guard_type g{ lock_ };
			h_ = h_->next;
		}
		else
		{
			h_ = h_->next;
		}

		return *this;
	}

	intrusive_list_iterator& operator--() TA_NO_THREAD_SAFETY_ANALYSIS
	{
		if constexpr (EnableLock)
		{
			lock_guard_type g{ lock_ };
			h_ = h_->prev;
		}
		else
		{
			h_ = h_->prev;
		}

		return *this;
	}

	intrusive_list_iterator operator++(dummy_type) noexcept
	{
		intrusive_list_iterator rc(*this);
		operator++();
		return rc;
	}

	intrusive_list_iterator operator--(dummy_type) noexcept
	{
		intrusive_list_iterator rc(*this);
		operator--();
		return rc;
	}

 private:
	using lock_guard_type = lock::lock_guard<TMutex>;

	head_type* NONNULL h_;
	mutable mutex_type lock_;
};

template<typename T, typename TMut, list_link<T, TMut> T::*Link>
struct default_list_node_trait
{
	static list_link<T, TMut>& node_link(T& element)
	{
		return element.*Link;
	}

	static list_link<T, TMut>& node_link(T* NONNULL element)
	{
		return element->*Link;
	}

	static list_link<T, TMut>* NONNULL node_link_ptr(T& element)
	{
		return &node_link(element);
	}

	static list_link<T, TMut>* NONNULL node_link_ptr(T* NONNULL element)
	{
		return &node_link(element);
	}
};

template<typename T>
struct default_list_deleter
{
	void operator()([[maybe_unused]]T* NONNULL ptr)
	{
		// do nothing
	}
};

template<typename T>
struct operator_delete_list_deleter
{
	void operator()(T* NONNULL ptr)
	{
		delete ptr;
	}
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
	NodeTrait<T, TMutex> Trait,
	bool EnableLock = false,
	Deleter<T> DeleterType = default_list_deleter<T>>

class intrusive_list
{
 public:
	using value_type = T;
	using mutex_type = TMutex;
	using head_type = list_link<value_type, mutex_type>;
	using size_type = size_t;
	using container_type = intrusive_list;
	using iterator_type = intrusive_list_iterator<T, TMutex, container_type, EnableLock>;
	using riterator_type = kbl::reversed_iterator<iterator_type>;
	using const_iterator_type = const iterator_type;

 public:
	/// New empty list
	constexpr intrusive_list() TA_NO_THREAD_SAFETY_ANALYSIS
	{
		if constexpr (EnableLock)
		{
			lock_guard_type g{ lock_ };
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
	intrusive_list(intrusive_list&& another) noexcept TA_NO_THREAD_SAFETY_ANALYSIS
	{
		if constexpr (EnableLock)
		{
			lock_guard_type g{ lock_ };
			list_init(&head_);

			head_type* iter = nullptr;
			head_type* t = nullptr;

			list_for_safe(iter, t, &another.head_)
			{
				list_remove_init(iter);
				list_add(iter, &this->head_);
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
				list_remove_init(iter);
				list_add(iter, this->head_);
			}

			size_ = another.size_;
			another.size_ = 0;
		}
	}

	/// First element
	/// \return the reference to first element
	T& front() TA_NO_THREAD_SAFETY_ANALYSIS
	{
		return *head_.next->parent;
	}

	T* NULLABLE front_ptr() TA_NO_THREAD_SAFETY_ANALYSIS
	{
		return head_.next->parent;
	}

	/// Last element
	/// \return the reference to first element
	T& back() TA_NO_THREAD_SAFETY_ANALYSIS
	{
		return *head_.prev->parent;
	}

	T* NULLABLE back_ptr() TA_NO_THREAD_SAFETY_ANALYSIS
	{
		return head_.prev->parent;
	}

	iterator_type begin() TA_NO_THREAD_SAFETY_ANALYSIS
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

	void insert(const_iterator_type iter, T* NONNULL item) TA_NO_THREAD_SAFETY_ANALYSIS
	{
		if constexpr (EnableLock)
		{
			lock_guard_type g{ lock_ };
			list_add(Trait::node_link_ptr(item), iter.h_);
			++size_;
		}
		else
		{
			list_add(Trait::node_link_ptr(item), iter.h_);
			++size_;
		}
	}

	void insert(const_iterator_type iter, T& item)
	{
		insert(iter, &item);
	}

	void insert(riterator_type iter, T& item)
	{
		insert(iter.get_iterator(), item);
	}

	void insert(riterator_type iter, T* NONNULL item)
	{
		insert(iter.get_iterator(), item);
	}

	void erase(iterator_type it) TA_NO_THREAD_SAFETY_ANALYSIS
	{
		if (list_empty(&head_))
		{
			return;
		}

		if constexpr (EnableLock)
		{
			lock_guard_type g{ lock_ };

			list_remove_init(it.h_);
			deleter_(it.h_->parent);
			--size_;
		}
		else
		{
			list_remove_init(it.h_);
			deleter_(it.h_->parent);
			--size_;
		}
	}

	void erase(riterator_type it)
	{
		erase(it.get_iterator());
	}

	/// Remove item by value. **it takes constant time**
	/// \param val
	void remove(T* NONNULL val) TA_NO_THREAD_SAFETY_ANALYSIS
	{
		if (list_empty(&head_))
		{
			return;
		}

		if constexpr (EnableLock)
		{
			lock_guard_type g{ lock_ };
			list_remove_init(Trait::node_link_ptr(val));
			deleter_(val);
			--size_;
		}
		else
		{
			list_remove_init(Trait::node_link_ptr(val));
			deleter_(val);
			--size_;
		}

	}

	void remove(T& val)
	{
		remove(&val);
	}

	void pop_back() TA_NO_THREAD_SAFETY_ANALYSIS
	{
		if (list_empty(&head_))
		{
			return;
		}

		if constexpr (EnableLock)
		{
			lock_guard_type g{ lock_ };
			auto entry = head_.prev;
			list_remove_init(entry);
			--size_;

			deleter_(entry->parent);
		}
		else
		{
			auto entry = head_.prev;
			list_remove_init(entry);
			--size_;

			deleter_(entry->parent);

		}

	}

	void push_back(T* NONNULL item) TA_NO_THREAD_SAFETY_ANALYSIS
	{
		if constexpr (EnableLock)
		{
			lock_guard_type g{ lock_ };
			list_add_tail(Trait::node_link_ptr(item), &head_);
			++size_;
		}
		else
		{
			list_add_tail(Trait::node_link_ptr(item), &head_);
			++size_;
		}

	}

	void push_back(T& item)
	{
		push_back(&item);
	}

	void pop_front() TA_NO_THREAD_SAFETY_ANALYSIS
	{
		if (list_empty(&head_))
		{
			return;
		}

		if constexpr (EnableLock)
		{
			lock_guard_type g{ lock_ };
			auto entry = head_.next;
			list_remove_init(entry);
			--size_;

			deleter_(entry->parent);
		}
		else
		{
			auto entry = head_.next;
			list_remove_init(entry);
			--size_;

			deleter_(entry->parent);

		}

	}

	void push_front(T* NONNULL item) TA_NO_THREAD_SAFETY_ANALYSIS
	{
		if constexpr (EnableLock)
		{
			lock_guard_type g{ lock_ };
			list_add(Trait::node_link_ptr(item), &head_);
			++size_;
		}
		else
		{
			list_add(Trait::node_link_ptr(item), &head_);
			++size_;
		}

	}

	void push_front(T& item)
	{
		push_front(&item);
	}

	void clear() TA_NO_THREAD_SAFETY_ANALYSIS
	{
		if constexpr (EnableLock)
		{
			lock_guard_type g{ lock_ };
			do_clear();
		}
		else
		{
			do_clear();
		}
	}

	/// swap this and another
	/// \param another
	void swap(container_type& another) noexcept TA_NO_THREAD_SAFETY_ANALYSIS
	{
		if constexpr (EnableLock)
		{
			lock_guard_type g{ lock_ };
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
	/// \tparam Compare cmp_(a,b) returns true if a comes before b
	/// \param another
	/// \param cmp
	template<typename Compare>
	void merge(container_type& another, Compare cmp) TA_NO_THREAD_SAFETY_ANALYSIS
	{
		if constexpr (EnableLock)
		{
			lock_guard_type g1{ lock_ };
			lock_guard_type g2{ another.lock_ };

			do_merge(another, cmp);
		}
		else
		{
			do_merge(another, cmp);
		}
	}

	/// join two lists
	/// \param other
	void splice(intrusive_list& other) TA_NO_THREAD_SAFETY_ANALYSIS
	{
		if constexpr (EnableLock)
		{
			lock_guard_type g{ lock_ };

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
	void splice(const_iterator_type pos, intrusive_list& other) TA_NO_THREAD_SAFETY_ANALYSIS
	{
		if constexpr (EnableLock)
		{
			lock_guard_type g{ lock_ };

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
		splice(pos.get_iterator(), other);
	}

	[[nodiscard]] size_type size() const
	{
		return size_;
	}

	[[nodiscard]] size_type size_slow() const TA_NO_THREAD_SAFETY_ANALYSIS
	{
		if constexpr (EnableLock)
		{
			lock_guard_type g{ lock_ };
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

	void do_clear()
	{
		if (!list_empty(&head_))
		{
			head_type* iter = nullptr, * t = nullptr;
			list_for_safe(iter, t, &head_)
			{
				list_remove(iter);

				if (iter->parent)
				{
					deleter_(iter->parent);
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
		if (head_ == another.head_)
		{
			return;
		}

		size_ += another.size_;

		head_type t_head{ nullptr };
		head_type* i1 = head_.next, * i2 = another.head_.next;
		while (i1 != &head_ && i2 != &another.head_)
		{
			if (cmp(*(i1->parent), *(i2->parent)))
			{
				auto next = i1->next;
				list_remove_init(i1);
				list_add_tail(i1, &t_head);
				i1 = next;
			}
			else
			{
				auto next = i2->next;
				list_remove_init(i2);
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
	static inline void util_list_init(head_type* NONNULL head)
	{
		head->next = head;
		head->prev = head;
	}

	static inline void
	util_list_add(head_type* NONNULL newnode, head_type* NONNULL prev, head_type* NONNULL next)
	{
		prev->next = newnode;
		next->prev = newnode;

		newnode->next = next;
		newnode->prev = prev;
	}

	static inline void util_list_remove(head_type* NONNULL prev, head_type* NONNULL next)
	{
		prev->next = next;
		next->prev = prev;
	}

	static inline void util_list_remove_entry(head_type* NONNULL entry)
	{
		util_list_remove(entry->prev, entry->next);
	}

	static inline void
	util_list_splice(const head_type* NONNULL list, head_type* NONNULL prev, head_type* NONNULL next)
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
	static inline void list_init(head_type* NONNULL head) TA_NO_THREAD_SAFETY_ANALYSIS
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
	static inline void list_add(head_type* NONNULL newnode, head_type* NONNULL head) TA_NO_THREAD_SAFETY_ANALYSIS
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
	static inline void list_add_tail(head_type* NONNULL newnode, head_type* NONNULL head) TA_NO_THREAD_SAFETY_ANALYSIS
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
	static inline void list_remove(head_type* NONNULL entry) TA_NO_THREAD_SAFETY_ANALYSIS
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
	static inline void list_remove_init(head_type* NONNULL entry) TA_NO_THREAD_SAFETY_ANALYSIS
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
	static inline void list_replace(head_type* NONNULL old, head_type* NONNULL newnode) TA_NO_THREAD_SAFETY_ANALYSIS
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
	static inline bool list_empty(const head_type* NONNULL head) TA_NO_THREAD_SAFETY_ANALYSIS
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
	static inline void list_swap(head_type* NONNULL e1, head_type* NONNULL e2) TA_NO_THREAD_SAFETY_ANALYSIS
	{
		if constexpr (EnableLock && !LockHeld)
		{
			lock_guard_type g1{ e1->lock };
			lock_guard_type g2{ e2->lock };

			auto* pos = e2->prev;
			list_remove_init<true>(e2);
			list_replace < true > (e1, e2);

			if (pos == e1)
			{
				pos = e2;
			}

			list_add < true > (e1, pos);
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
	static inline void list_splice(head_type* NONNULL list, head_type* NONNULL head) TA_NO_THREAD_SAFETY_ANALYSIS
	{
		if constexpr (EnableLock)
		{
			lock_guard_type g1{ list->lock };
			lock_guard_type g2{ head->lock };

			if (!list_empty < true > (list))
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
	static inline void list_splice_init(head_type* NONNULL list, head_type* NONNULL head) TA_NO_THREAD_SAFETY_ANALYSIS
	{
		if constexpr (EnableLock)
		{
			lock_guard_type g1{ list->lock };
			lock_guard_type g2{ head->lock };

			if (!list_empty < true > (list))
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

	using lock_guard_type = lock::lock_guard<TMutex>;

	head_type head_   TA_GUARDED(lock_) { nullptr };

	size_type size_{ 0 };

	mutable DeleterType deleter_;

	mutable mutex_type lock_;
};

#undef list_for
#undef list_for_safe

#pragma pop_macro("list_for")
#pragma pop_macro("list_for_safe")

template<typename T,
	typename TMutex,
	list_link<T, TMutex> T::*Link,
	bool EnableLock = false,
	Deleter<T> DeleterType=default_list_deleter<T>>

using intrusive_list_with_default_trait = intrusive_list<T,
                                                         TMutex,
                                                         default_list_node_trait<T, TMutex, Link>,
                                                         EnableLock,
                                                         DeleterType>;

} // namespace


#pragma pop_macro("KDEBUG_ASSERT")
