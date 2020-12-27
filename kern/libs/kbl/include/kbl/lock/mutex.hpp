#pragma once
#include "kbl/lock/lockable.hpp"

namespace lock
{
	class mutex: public lockable
	{
	 public:
		virtual ~mutex() = default;

		virtual bool holding() noexcept = 0;
	};
}