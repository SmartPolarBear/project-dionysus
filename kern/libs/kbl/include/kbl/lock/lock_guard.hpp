#pragma once

#include "debug/thread_annotations.hpp"
#include "kbl/lock/lockable.hpp"

#include <tuple>

namespace lock
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

/// \brief  RAII wrapper for automatically locking and unlocking the lock
/// \tparam TMutex which satisfies BasicLockable
template<BasicLockable TMutex>
class TA_SCOPED_CAP lock_guard //<TMutex>
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

	[[nodiscard]]explicit lock_guard(adopt_lock_tag, mutex_type& _m) noexcept TA_REQ(_m)
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

}
