#pragma once
#include "ktl/mutex/mutex_concepts.hpp"

#include "debug/thread_annotations.hpp"

namespace ktl
{
	namespace mutex
	{
		/// \brief Automatically lock and unlock spinlock_struct
		/// \tparam TMutex which satisfies BasicLockable
		template<BasicLockable TMutex>
		class TA_SCOPED_CAP lock_guard
		{
		 public:
			typedef TMutex mutex_type;

			[[nodiscard]]explicit lock_guard(mutex_type& _m) noexcept TA_ACQ(_m)
				: m(&_m)
			{
				m->lock();
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
}