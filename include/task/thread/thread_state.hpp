#pragma once

#include "ktl/pair.hpp"

#include "debug/kdebug.h"

namespace task
{
	class thread_state final
	{
	 public:
		enum class [[clang::enum_extensibility(closed)]] lifecycle : uint32_t
		{
			INITIAL,
			INITIALIZED,

			RUNNING,
			SUSPENDED,

			DYING,
			DEAD
		};

		enum class [[clang::enum_extensibility(closed)]] exception : uint32_t
		{
			IDLE,
			UNPROCESSED,
			TRY_NEXT,
			RESUME
		};

		[[nodiscard]] lifecycle get_lifecycle() const
		{
			switch (value_)
			{
			case value::INITIAL_IDLE:
				return lifecycle::INITIAL;
			case value::INITIALIZED_IDLE:
				return lifecycle::INITIALIZED;
			case value::RUNNING_IDLE:
			case value::RUNNING_UNPROCESSED:
			case value::RUNNING_TRY_NEXT:
			case value::RUNNING_RESUME:
				return lifecycle::RUNNING;
			case value::SUSPENDED_IDLE:
			case value::SUSPENDED_UNPROCESSED:
			case value::SUSPENDED_TRY_NEXT:
			case value::SUSPENDED_RESUME:
				return lifecycle::SUSPENDED;
			case value::DYING_IDLE:
			case value::DYING_UNPROCESSED:
			case value::DYING_TRY_NEXT:
			case value::DYING_RESUME:
				return lifecycle::DYING;
			case value::DEAD_IDLE:
				return lifecycle::DEAD;
			default:
				KDEBUG_ASSERT(false);
			}
		}

		[[nodiscard]] exception get_exception() const
		{
			switch (value_)
			{
			case value::RUNNING_IDLE:
			case value::SUSPENDED_IDLE:
			case value::DYING_IDLE:
				return exception::IDLE;
			case value::RUNNING_UNPROCESSED:
			case value::SUSPENDED_UNPROCESSED:
			case value::DYING_UNPROCESSED:
				return exception::UNPROCESSED;
			case value::RUNNING_TRY_NEXT:
			case value::SUSPENDED_TRY_NEXT:
			case value::DYING_TRY_NEXT:
				return exception::TRY_NEXT;
			case value::RUNNING_RESUME:
			case value::SUSPENDED_RESUME:
			case value::DYING_RESUME:
				return exception::RESUME;
			case value::INITIALIZED_IDLE:
			case value::DEAD_IDLE:
				// Someone could have, for example, requested zx_info_thread_t.
				return exception::IDLE;
			default:
				KDEBUG_ASSERT(false);
			}
		}

		void set(lifecycle lc)
		{
			switch (lc)
			{
			case lifecycle::INITIAL:
				KDEBUG_ASSERT(false);
				return;
			case lifecycle::INITIALIZED:
				switch (value_)
				{
				case value::INITIAL_IDLE:
					value_ = value::INITIALIZED_IDLE;
					return;
				default:
					KDEBUG_ASSERT(false);
					return;
				}
			case lifecycle::RUNNING:
				switch (value_)
				{
				case value::INITIALIZED_IDLE:
				case value::SUSPENDED_IDLE:
					value_ = value::RUNNING_IDLE;
					return;
				case value::SUSPENDED_UNPROCESSED:
					value_ = value::RUNNING_UNPROCESSED;
					return;
				case value::SUSPENDED_TRY_NEXT:
					value_ = value::RUNNING_TRY_NEXT;
					return;
				case value::SUSPENDED_RESUME:
					value_ = value::RUNNING_RESUME;
					return;
				default:
					KDEBUG_ASSERT(false);
					return;
				}
			case lifecycle::SUSPENDED:
				switch (value_)
				{
				case value::RUNNING_IDLE:
					value_ = value::SUSPENDED_IDLE;
					return;
				case value::RUNNING_UNPROCESSED:
					value_ = value::SUSPENDED_UNPROCESSED;
					return;
				case value::RUNNING_TRY_NEXT:
					value_ = value::SUSPENDED_TRY_NEXT;
					return;
				case value::RUNNING_RESUME:
					value_ = value::SUSPENDED_RESUME;
					return;
				default:
					KDEBUG_ASSERT(false);
					return;
				}
			case lifecycle::DYING:
				switch (value_)
				{
				case value::RUNNING_IDLE:
				case value::SUSPENDED_IDLE:
				case value::DYING_IDLE:
					value_ = value::DYING_IDLE;
					return;
				case value::RUNNING_UNPROCESSED:
				case value::SUSPENDED_UNPROCESSED:
				case value::DYING_UNPROCESSED:
					value_ = value::DYING_UNPROCESSED;
					return;
				case value::RUNNING_TRY_NEXT:
				case value::SUSPENDED_TRY_NEXT:
				case value::DYING_TRY_NEXT:
					value_ = value::DYING_TRY_NEXT;
					return;
				case value::RUNNING_RESUME:
				case value::SUSPENDED_RESUME:
				case value::DYING_RESUME:
					value_ = value::DYING_RESUME;
					return;
				default:
					KDEBUG_ASSERT(false);
					return;
				}
			case lifecycle::DEAD:
				switch (value_)
				{
				case value::DYING_IDLE:
				case value::DYING_UNPROCESSED:
				case value::DYING_TRY_NEXT:
				case value::DYING_RESUME:
					value_ = value::DEAD_IDLE;
					return;
				default:
					KDEBUG_ASSERT(false);
					return;
				}
			}
		}

		void set(exception ext)
		{
			switch (ext)
			{
			case exception::IDLE:
				switch (value_)
				{
				case value::RUNNING_UNPROCESSED:
				case value::RUNNING_TRY_NEXT:
				case value::RUNNING_RESUME:
					value_ = value::RUNNING_IDLE;
					return;
				case value::SUSPENDED_UNPROCESSED:
				case value::SUSPENDED_TRY_NEXT:
				case value::SUSPENDED_RESUME:
					value_ = value::SUSPENDED_IDLE;
					return;
				case value::DYING_UNPROCESSED:
				case value::DYING_TRY_NEXT:
				case value::DYING_RESUME:
					value_ = value::DYING_IDLE;
					return;
				default:
					KDEBUG_ASSERT(false);
					return;
				}
			case exception::UNPROCESSED:
				switch (value_)
				{
				case value::RUNNING_IDLE:
					value_ = value::RUNNING_UNPROCESSED;
					return;
				case value::SUSPENDED_IDLE:
					value_ = value::SUSPENDED_UNPROCESSED;
					return;
				case value::DYING_IDLE:
					value_ = value::DYING_UNPROCESSED;
					return;
				default:
					KDEBUG_ASSERT(false);
					return;
				}
			case exception::TRY_NEXT:
				switch (value_)
				{
				case value::RUNNING_UNPROCESSED:
					value_ = value::RUNNING_TRY_NEXT;
					return;
				case value::SUSPENDED_UNPROCESSED:
					value_ = value::SUSPENDED_TRY_NEXT;
					return;
				case value::DYING_UNPROCESSED:
					value_ = value::DYING_TRY_NEXT;
					return;
				default:
					KDEBUG_ASSERT(false);
					return;
				}
			case exception::RESUME:
				switch (value_)
				{
				case value::RUNNING_UNPROCESSED:
					value_ = value::RUNNING_RESUME;
					return;
				case value::SUSPENDED_UNPROCESSED:
					value_ = value::SUSPENDED_RESUME;
					return;
				case value::DYING_UNPROCESSED:
					value_ = value::DYING_RESUME;
					return;
				default:
					KDEBUG_ASSERT(false);
					return;
				}
			}
		}

	 private:

		/// \brief valid compositions of lifecycle and exception
		enum class [[clang::enum_extensibility(closed)]] value : uint64_t
		{
			INITIAL_IDLE,

			INITIALIZED_IDLE,

			RUNNING_IDLE,
			RUNNING_UNPROCESSED,
			RUNNING_TRY_NEXT,
			RUNNING_RESUME,

			SUSPENDED_IDLE,
			SUSPENDED_UNPROCESSED,
			SUSPENDED_TRY_NEXT,
			SUSPENDED_RESUME,

			DYING_IDLE,
			DYING_UNPROCESSED,
			DYING_TRY_NEXT,
			DYING_RESUME,

			DEAD_IDLE,
		};

		value value_ = value::INITIAL_IDLE;
	};
}