#pragma once
#include "ktl/mutex/mutex_concepts.hpp"

namespace ktl
{
	namespace mutex
	{
		/// \brief Automatically lock and unlock
		/// \tparam TMutex
		template<BasicLockable TMutex>
		class lock_guard
		{
		 public:
			typedef TMutex mutex_type;

			explicit lock_guard(mutex_type& _m) noexcept
				: m(&_m)
			{
				m->lock();
			}

			~lock_guard() noexcept
			{
				m->unlock();
			}

			lock_guard(lock_guard const&) = delete;
			lock_guard& operator=(lock_guard const&) = delete;

		 private:
			mutex_type* m;
		};
	}
}