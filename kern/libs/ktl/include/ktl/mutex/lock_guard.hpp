#pragma once
#include "ktl/mutex/mutex_concepts.hpp"

#include "debug/thread_annotations.hpp"

#include <tuple>

namespace ktl
{
namespace mutex
{

/// \brief empty tag struct for defer_lock
struct defer_lock_tag
{
	explicit defer_lock_tag() = default;
};

/// \brief empty tag struct for try_to_lock
struct try_to_lock_tag
{
	explicit try_to_lock_tag() = default;
};

/// \brief empty tag struct for adopt_lock
struct adopt_lock_tag
{
	explicit adopt_lock_tag() = default;
};

inline constexpr defer_lock_tag defer_lock{};
inline constexpr try_to_lock_tag try_to_lock{};
inline constexpr adopt_lock_tag adopt_lock{};

template<BasicLockable ...Mutexes>
class TA_SCOPED_CAP lock_guard
{
 public:
	[[nodiscard]]explicit lock_guard(Mutexes& ...mutexes)  TA_ACQ(mutexes): mutexes_(mutexes...)
	{
		std::apply([](Mutexes& ... mut)
		{
		  ((void)mut.lock(), ...);
		}, mutexes_);
	}

	[[nodiscard]]explicit lock_guard(try_to_lock_tag, Mutexes& ...mutexes)  TA_ACQ(mutexes): mutexes_(mutexes...)
	{
		std::apply([](Mutexes& ... mut)
		{
		  ((void)mut.try_lock(), ...);
		}, mutexes_);
	}

	[[nodiscard]]explicit lock_guard(adopt_lock_tag, Mutexes& ...mutexes)  TA_ACQ(mutexes): mutexes_(mutexes...)
	{
	}

	~lock_guard() noexcept TA_REL()
	{
		std::apply([](Mutexes& ... mut)
		{
		  (..., (void)mut.unlock());
		}, mutexes_);
	}

	void unlock() noexcept TA_REL()
	{
		std::apply([](Mutexes& ... mut)
		{
		  ((void)mut.lock(), ...);
		}, mutexes_);
	}

	lock_guard(lock_guard const&) = delete;
	lock_guard& operator=(lock_guard const&) = delete;

 private:
	std::tuple<Mutexes& ...> mutexes_;
};

/// \brief  RAII wrapper for automatically locking and unlocking the lock
/// \tparam TMutex which satisfies BasicLockable
template<BasicLockable TMutex>
class TA_SCOPED_CAP lock_guard<TMutex>
{
 public:
	typedef TMutex mutex_type;

	[[nodiscard]]explicit lock_guard(mutex_type& _m) noexcept TA_ACQ(_m)
		: m(&_m)
	{
		m->lock();
	}

	[[nodiscard]]explicit lock_guard(try_to_lock_tag, mutex_type& _m) noexcept TA_ACQ(_m)
		: m(&_m)
	{
		m->try_lock();
	}

	[[nodiscard]]explicit lock_guard(adopt_lock_tag, mutex_type& _m) noexcept TA_ACQ(_m)
		: m(&_m)
	{
	}

	~lock_guard() noexcept TA_REL()
	{
		m->unlock();
	}

	void unlock() noexcept TA_REL()
	{
		m->unlock();
	}

	lock_guard(lock_guard const&) = delete;
	lock_guard& operator=(lock_guard const&) = delete;

 private:
	mutex_type* m;
};

template<>
class lock_guard<>
{
 public:
	explicit lock_guard()
	{
	}
	explicit lock_guard(adopt_lock_tag)
	{
	}
	~lock_guard() noexcept
	{
	}

	lock_guard(const lock_guard&) = delete;
	lock_guard& operator=(const lock_guard&) = delete;
};

}

}