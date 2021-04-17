#pragma once
#include "debug/nullability.hpp"

#include "kbl/lock/lock_guard.hpp"

#include "kbl/data/utility.hpp"

#include "ktl/concepts.hpp"

#include <iterator>

#pragma push_macro("KDEBUG_ASSERT")

//#define ENABLE_DEBUG_MACRO
#ifndef ENABLE_DEBUG_MACRO
#ifdef KDEBUG_ASSERT
#undef KDEBUG_ASSERT
#endif
#define KDEBUG_ASSERT(...)
#endif

//#define ENABLE_NULLABILITY_CHECK
#ifndef ENABLE_NULLABILITY_CHECK
#ifdef NULLABLE
#undef NULLABLE
#endif

#ifdef NONNULL
#undef NONNULL
#endif

#define NULLABLE
#define NONNULL

#endif


namespace kbl
{

template<typename T, typename E>
concept Deleter =
requires(T del, E *ptr)
{
	del(ptr);
};

// linked list head
template<typename TParent, typename TMutex>
class list_link
{
 public:

	list_link() : parent_{nullptr}, next_{this}, prev_{this}
	{
	}

	explicit list_link(TParent * p) : parent_{p}, next_{this}, prev_{this}
	{
	}

	explicit list_link(TParent &p) : parent_{&p}, next_{this}, prev_{this}
	{
	}

	list_link(list_link &&another) noexcept
		: parent_(std::move(another.parent_)),
		  next_(std::move(another.next_)),
		  prev_(std::move(another.prev_))
	{
		next_->prev_ = this;
		prev_->next_ = this;

		//put another in a valid empty state
		another.parent_ = nullptr;
		another.next_ = &another;
		another.prev_ = &another;
	}

	list_link(const list_link &another)
		: parent_(another.parent_),
		  next_(another.next_),
		  prev_(another.prev_)
	{
	}

	list_link &operator=(const list_link &another)
	{
		if (this == &another)
		{
			return *this;
		}

		parent_ = another.parent_;
		next_ = another.next_;
		prev_ = another.prev_;

		return *this;
	}

	bool operator==(const list_link &that) const
	{
		return parent_ == that.parent_ &&
			next_ == that.next_ &&
			prev_ == that.prev_;
	}

	bool operator!=(const list_link &that) const
	{
		return !(*this == that);
	}

	[[nodiscard]] bool is_empty_or_detached() const
	{
		return next_ == this && prev_ == this;
	}

	[[nodiscard]] bool is_head() const
	{
		return !parent_;
	}

	[[nodiscard]] bool is_valid() const
	{
		return next_ != nullptr && prev_ != nullptr;
	}

 public:
	TParent * parent_;
	list_link * next_, * prev_;

	mutable TMutex lock_;
};

template<typename T, typename U, typename Mutex>
concept NodeTrait =
requires(U &u)
{
	{ T::node_link(u) }->ktl::convertible_to<list_link<U, Mutex> &>;
	{ T::node_link(&u) }->ktl::convertible_to<list_link<U, Mutex> &>;
	{ T::node_link_ptr(u) }->ktl::convertible_to<list_link<U, Mutex> *>;
	{ T::node_link_ptr(&u) }->ktl::convertible_to<list_link<U, Mutex> *>;
};

template<typename T, typename TMutex, class Container, bool EnableLock = false>
class intrusive_list_iterator
{
 public:
	friend Container;

	using value_type = T;

	using reference = T &;
	using pointer = T *;

	using difference_type = std::ptrdiff_t;

	using iterator_category = std::input_iterator_tag;

	using mutex_type = TMutex;
	using head_type = list_link<value_type, mutex_type>;

	using dummy_type = int;

 public:
	constexpr intrusive_list_iterator() = default;
	~intrusive_list_iterator() = default;

	constexpr explicit intrusive_list_iterator(head_type * h) : h_(h)
	{
	}

	constexpr intrusive_list_iterator(intrusive_list_iterator &&another) noexcept
	{
		h_ = another.h_;
		another.h_ = nullptr;
	}

	constexpr intrusive_list_iterator(const intrusive_list_iterator &another)
	{
		h_ = another.h_;
	}

	intrusive_list_iterator &operator=(const intrusive_list_iterator &another) TA_NO_THREAD_SAFETY_ANALYSIS
	{
		if (this == &another)
		{
			return *this;
		}

		if constexpr(EnableLock)
		{
			lock_guard_type g{lock_};
			h_ = another.h_;
		}
		else
		{
			h_ = another.h_;

		}

		return *this;
	}

	// C++ named requirements: Swappable
	void swap(intrusive_list_iterator &other)
	{
		std::swap(h_, other.h_);
	}

	reference operator*()
	{
		return *operator->();
	}

	pointer  operator->()
	{
		return h_->parent_;
	}

	constexpr intrusive_list_iterator &operator++() TA_NO_THREAD_SAFETY_ANALYSIS
	{
		if constexpr (EnableLock)
		{
			lock_guard_type g{lock_};
			h_ = h_->next_;
		}
		else
		{
			h_ = h_->next_;
		}

		return *this;
	}

	constexpr intrusive_list_iterator &operator--() TA_NO_THREAD_SAFETY_ANALYSIS
	{
		if constexpr (EnableLock)
		{
			lock_guard_type g{lock_};
			h_ = h_->prev_;
		}
		else
		{
			h_ = h_->prev_;
		}

		return *this;
	}

	constexpr intrusive_list_iterator operator++(dummy_type) noexcept
	{
		intrusive_list_iterator rc(*this);
		operator++();
		return rc;
	}

	constexpr intrusive_list_iterator operator--(dummy_type) noexcept
	{
		intrusive_list_iterator rc(*this);
		operator--();
		return rc;
	}

	friend constexpr bool operator==(const intrusive_list_iterator &lhs, const intrusive_list_iterator &rhs) noexcept
	{
		return lhs.h_ == rhs.h_;

	}

	friend constexpr bool operator!=(const intrusive_list_iterator &lhs, const intrusive_list_iterator &rhs) noexcept
	{
		return !(lhs == rhs);
	}

 private:
	using lock_guard_type = lock::lock_guard<TMutex>;

	head_type * h_;
	mutable mutex_type lock_;
};

template<typename T, typename TMut, list_link<T, TMut> T::*Link>
struct default_list_node_trait
{
	static list_link<T, TMut> &node_link(T &element)
	{
		return element.*Link;
	}

	static list_link<T, TMut> &node_link(T * element)
	{
		return element->*Link;
	}

	static list_link<T, TMut> * node_link_ptr(T &element)
	{
		return &node_link(element);
	}

	static list_link<T, TMut> * node_link_ptr(T * element)
	{
		return &node_link(element);
	}
};

template<typename T>
struct default_list_deleter
{
	void operator()([[maybe_unused]]T * ptr)
	{
		// do nothing
	}
};

template<typename T>
struct operator_delete_list_deleter
{
	void operator()(T * ptr)
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
    for ((pos) = (head)->next_; (pos) != (head); (pos) = (pos)->next_)

#define list_for_safe(pos, n, head) \
    for (pos = (head)->next_, n = pos->next_; pos != (head); pos = n, n = pos->next_)

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
			lock_guard_type g{lock_};
			list_init(&head_);
		}
		else
		{
			list_init(&head_);
		}
	}

	/// Isn't copiable
	intrusive_list(const intrusive_list &) = delete;

	intrusive_list &operator=(const intrusive_list &) = delete;

	/// Move constructor
	intrusive_list(intrusive_list &&another) noexcept
		: head_(std::move(another.head_)),
		  size_(std::move(another.size_))
	{
		if constexpr (EnableLock)
		{
			lock_guard_type g{another.lock_};
			another.size_ = 0;
		}
		else
		{
			another.size_ = 0;
		}
	}

	/// First element
	/// \return the reference to first element
	T &front() TA_NO_THREAD_SAFETY_ANALYSIS
	{
		return *head_.next_->parent_;
	}

	T * front_ptr() TA_NO_THREAD_SAFETY_ANALYSIS
	{
		return head_.next_->parent_;
	}

	/// Last element
	/// \return the reference to first element
	T &back() TA_NO_THREAD_SAFETY_ANALYSIS
	{
		return *head_.prev_->parent_;
	}

	T * back_ptr() TA_NO_THREAD_SAFETY_ANALYSIS
	{
		return head_.prev_->parent_;
	}

	iterator_type begin() TA_NO_THREAD_SAFETY_ANALYSIS
	{
		return iterator_type{head_.next_};
	}

	iterator_type end() TA_NO_THREAD_SAFETY_ANALYSIS
	{
		return iterator_type{&head_};
	}

	const_iterator_type cbegin() TA_NO_THREAD_SAFETY_ANALYSIS
	{
		return const_iterator_type{head_.next_};
	}

	const_iterator_type cend() TA_NO_THREAD_SAFETY_ANALYSIS
	{
		return const_iterator_type{&head_};
	}

	riterator_type rbegin() TA_NO_THREAD_SAFETY_ANALYSIS
	{
		return riterator_type{head_.prev_};
	}

	riterator_type rend()TA_NO_THREAD_SAFETY_ANALYSIS
	{
		return riterator_type{&head_};
	}

	void insert(const_iterator_type iter, T * item) TA_NO_THREAD_SAFETY_ANALYSIS
	{
		if constexpr (EnableLock)
		{
			lock_guard_type g{lock_};
			list_add(Trait::node_link_ptr(item), iter.h_);
			++size_;
		}
		else
		{
			list_add(Trait::node_link_ptr(item), iter.h_);
			++size_;
		}
	}

	void insert(const_iterator_type iter, T &item)
	{
		insert(iter, &item);
	}

	void insert(riterator_type iter, T &item)
	{
		insert(iter.get_iterator(), item);
	}

	void insert(riterator_type iter, T * item)
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
			lock_guard_type g{lock_};

			list_remove_init(it.h_);
			deleter_(it.h_->parent_);
			--size_;
		}
		else
		{
			list_remove_init(it.h_);
			deleter_(it.h_->parent_);
			--size_;
		}
	}

	void erase(riterator_type it)
	{
		erase(it.get_iterator());
	}

	/// Remove item by value. **it takes constant time**
	/// \param val
	void remove(T * val) TA_NO_THREAD_SAFETY_ANALYSIS
	{
		if (list_empty(&head_))
		{
			return;
		}

		if constexpr (EnableLock)
		{
			lock_guard_type g{lock_};
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

	void remove(T &val)
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
			lock_guard_type g{lock_};
			auto entry = head_.prev_;
			list_remove_init(entry);
			--size_;

			deleter_(entry->parent_);
		}
		else
		{
			auto entry = head_.prev_;
			list_remove_init(entry);
			--size_;

			deleter_(entry->parent_);

		}

	}

	void push_back(T * item) TA_NO_THREAD_SAFETY_ANALYSIS
	{
		if constexpr (EnableLock)
		{
			lock_guard_type g{lock_};
			list_add_tail(Trait::node_link_ptr(item), &head_);
			++size_;
		}
		else
		{
			list_add_tail(Trait::node_link_ptr(item), &head_);
			++size_;
		}

	}

	void push_back(T &item)
	{
		push_back(&item);
	}

	void pop_front()
	{
		if (list_empty(&head_))
		{
			return;
		}

		if constexpr (EnableLock)
		{
			lock_guard_type g{lock_};
			auto entry = head_.next_;
			list_remove_init(entry);
			--size_;

			deleter_(entry->parent_);
		}
		else
		{
			auto entry = head_.next_;
			list_remove_init(entry);
			--size_;

			deleter_(entry->parent_);

		}

	}

	void push_front(T * item)
	{
		if constexpr (EnableLock)
		{
			lock_guard_type g{lock_};
			list_add(Trait::node_link_ptr(item), &head_);
			++size_;
		}
		else
		{
			list_add(Trait::node_link_ptr(item), &head_);
			++size_;
		}

	}

	void push_front(T &item)
	{
		push_front(&item);
	}

	void clear()
	{
		if constexpr (EnableLock)
		{
			lock_guard_type g{lock_};
			do_clear();
		}
		else
		{
			do_clear();
		}
	}

	/// swap this and another
	/// \param another
	void swap(container_type &another) noexcept
	{
		if constexpr (EnableLock)
		{
			lock_guard_type g{lock_};
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
	void merge(container_type &another)
	{
		merge(another, [](const T &a, const T &b)
		{
		  return a < b;
		});
	}

	/// Merge two **sorted** list, after that another becomes empty
	/// \tparam Compare cmp_(a,b) returns true if a comes before b
	/// \param another
	/// \param cmp
	template<typename Compare>
	void merge(container_type &another, Compare cmp)
	{
		if constexpr (EnableLock)
		{
			lock_guard_type g1{lock_};
			lock_guard_type g2{another.lock_};

			do_merge(another, cmp);
		}
		else
		{
			do_merge(another, cmp);
		}
	}

	/// join two lists
	/// \param other
	void splice(intrusive_list &other)
	{
		if constexpr (EnableLock)
		{
			lock_guard_type g{lock_};

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
	void splice(const_iterator_type pos, intrusive_list &other)
	{
		if constexpr (EnableLock)
		{
			lock_guard_type g{lock_};

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
	void splice(riterator_type pos, intrusive_list &other)
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
			lock_guard_type g{lock_};
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

	void do_clear() TA_NO_THREAD_SAFETY_ANALYSIS
	{
		if (!list_empty(&head_))
		{
			head_type *iter = nullptr, *t = nullptr;
			list_for_safe(iter, t, &head_)
			{
				list_remove(iter);

				if (iter->parent_)
				{
					deleter_(iter->parent_);
				}
			}
		}
		size_ = 0;
	}

	size_type do_size_slow() const TA_NO_THREAD_SAFETY_ANALYSIS
	{
		size_type sz = 0;
		head_type *iter = nullptr;
		list_for(iter, &head_)
		{
			sz++;
		}
		return sz;
	}

	template<typename Compare>
	void do_merge(container_type &another, Compare cmp) TA_NO_THREAD_SAFETY_ANALYSIS
	{
		if (head_ == another.head_)
		{
			return;
		}

		size_ += another.size_;

		head_type t_head{nullptr};
		head_type *i1 = head_.next_, *i2 = another.head_.next_;
		while (i1 != &head_ && i2 != &another.head_)
		{
			if (cmp(*(i1->parent_), *(i2->parent_)))
			{
				auto next = i1->next_;
				list_remove_init(i1);
				list_add_tail(i1, &t_head);
				i1 = next;
			}
			else
			{
				auto next = i2->next_;
				list_remove_init(i2);
				list_add_tail(i2, &t_head);
				i2 = next;
			}
		}

		while (i1 != &head_)
		{
			auto next = i1->next_;
			list_remove_init(i1);
			list_add_tail(i1, &t_head);
			i1 = next;
		}

		while (i2 != &another.head_)
		{
			auto next = i2->next_;
			list_remove_init(i2);
			list_add_tail(i2, &t_head);
			i2 = next;
		}

		another.size_ = 0;
		list_swap(&head_, &t_head);
	}

 private:
	static inline void util_list_init(head_type * head)
	{
		head->next_ = head;
		head->prev_ = head;
	}

	static inline void
	util_list_add(head_type * newnode, head_type * prev, head_type * next)
	{
		prev->next_ = newnode;
		next->prev_ = newnode;

		newnode->next_ = next;
		newnode->prev_ = prev;
	}

	static inline void util_list_remove(head_type * prev, head_type * next)
	{
		prev->next_ = next;
		next->prev_ = prev;
	}

	static inline void util_list_remove_entry(head_type * entry)
	{
		util_list_remove(entry->prev_, entry->next_);
	}

	static inline void
	util_list_splice(const head_type * list, head_type * prev, head_type * next)
	{
		head_type *first = list->next_, *last = list->prev_;

		first->prev_ = prev;
		prev->next_ = first;

		last->next_ = next;
		next->prev_ = last;
	}

 private:

	// initialize the list
	template<bool LockHeld = false>
	static inline void list_init(head_type * head) TA_NO_THREAD_SAFETY_ANALYSIS
	{
		if constexpr (EnableLock && !LockHeld)
		{
			lock_guard_type g{head->lock_};
			util_list_init(head);
		}
		else
		{
			util_list_init(head);
		}
	}

// add the new node after the specified head_
	template<bool LockHeld = false>
	static inline void list_add(head_type * newnode, head_type * head) TA_NO_THREAD_SAFETY_ANALYSIS
	{
		if constexpr (EnableLock && !LockHeld)
		{
			lock_guard_type g1{newnode->lock_};
			lock_guard_type g2{head->lock_};

			util_list_add(newnode, head, head->next_);
		}
		else
		{
			util_list_add(newnode, head, head->next_);
		}
	}

// add the new node before the specified head_
	template<bool LockHeld = false>
	static inline void list_add_tail(head_type * newnode, head_type * head) TA_NO_THREAD_SAFETY_ANALYSIS
	{
		if constexpr (EnableLock && !LockHeld)
		{
			lock_guard_type g1{newnode->lock_};
			lock_guard_type g2{head->lock_};

			util_list_add(newnode, head->prev_, head);

		}
		else
		{
			util_list_add(newnode, head->prev_, head);

		}
	}

// delete the entry
	template<bool LockHeld = false>
	static inline void list_remove(head_type * entry) TA_NO_THREAD_SAFETY_ANALYSIS
	{
		if constexpr (EnableLock && !LockHeld)
		{
			lock_guard_type g1{entry->lock_};

			util_list_remove_entry(entry);

			entry->prev_ = nullptr;
			entry->next_ = nullptr;
		}
		else
		{
			util_list_remove_entry(entry);

			entry->prev_ = nullptr;
			entry->next_ = nullptr;
		}
	}

	template<bool LockHeld = false>
	static inline void list_remove_init(head_type * entry) TA_NO_THREAD_SAFETY_ANALYSIS
	{
		if constexpr (EnableLock && !LockHeld)
		{
			lock_guard_type g1{entry->lock_};

			util_list_remove_entry(entry);

			entry->prev_ = nullptr;
			entry->next_ = nullptr;

			util_list_init(entry);
		}
		else
		{
			util_list_remove_entry(entry);

			entry->prev_ = nullptr;
			entry->next_ = nullptr;

			util_list_init(entry);
		}
	}

// replace the old entry with newnode
	template<bool LockHeld = false>
	static inline void list_replace(head_type * old, head_type * newnode) TA_NO_THREAD_SAFETY_ANALYSIS
	{
		if constexpr (EnableLock && !LockHeld)
		{
			newnode->next_ = old->next_;
			newnode->next_->prev_ = newnode;
			newnode->prev_ = old->prev_;
			newnode->prev_->next_ = newnode;
		}
		else
		{
			newnode->next_ = old->next_;
			newnode->next_->prev_ = newnode;
			newnode->prev_ = old->prev_;
			newnode->prev_->next_ = newnode;
		}

	}

	template<bool LockHeld = false>
	static inline bool list_empty(const head_type * head) TA_NO_THREAD_SAFETY_ANALYSIS
	{
		if constexpr (EnableLock && !LockHeld)
		{
			lock_guard_type g{head->lock_};

			return (head->next_) == head;
		}
		else
		{
			return (head->next_) == head;
		}
	}

	template<bool LockHeld = false>
	static inline void list_swap(head_type * e1, head_type * e2) TA_NO_THREAD_SAFETY_ANALYSIS
	{
		if constexpr (EnableLock && !LockHeld)
		{
			lock_guard_type g1{e1->lock_};
			lock_guard_type g2{e2->lock_};

			auto *pos = e2->prev_;
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
			auto *pos = e2->prev_;
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
	static inline void list_splice(head_type * list, head_type * head) TA_NO_THREAD_SAFETY_ANALYSIS
	{
		if constexpr (EnableLock)
		{
			lock_guard_type g1{list->lock_};
			lock_guard_type g2{head->lock_};

			if (!list_empty<true>(list))
			{
				util_list_splice(list, head, head->next_);
			}
		}
		else
		{
			if (!list_empty(list))
			{
				util_list_splice(list, head, head->next_);
			}
		}
	}

	/// \tparam TParent
	/// \param list	the new list to add
	/// \param head the place to add it in the list
	static inline void list_splice_init(head_type * list, head_type * head) TA_NO_THREAD_SAFETY_ANALYSIS
	{
		if constexpr (EnableLock)
		{
			lock_guard_type g1{list->lock_};
			lock_guard_type g2{head->lock_};

			if (!list_empty<true>(list))
			{
				util_list_splice(list, head, head->next_);
				util_list_init(list);
			}
		}
		else
		{
			if (!list_empty(list))
			{
				util_list_splice(list, head, head->next_);
				util_list_init(list);
			}
		}
	}

	using lock_guard_type = lock::lock_guard<TMutex>;

	head_type head_ TA_GUARDED(lock_) {nullptr};

	size_type size_{0};

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
