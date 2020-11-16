#pragma once

namespace lock
{
	// C++ named requirements: BasicLockable
	// difference: we do not throw any exceptions, instead, we panic.
	class basic_lockable
	{
	 public:
		virtual ~basic_lockable() = default;

		virtual void lock() noexcept = 0;
		virtual void unlock() noexcept = 0;
	};

	class lockable
		: public basic_lockable
	{
	 public:
		virtual ~lockable() = default;

		virtual bool try_lock() noexcept = 0;
	};
}