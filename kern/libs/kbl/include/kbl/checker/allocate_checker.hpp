#pragma once
#include "debug/kdebug.h"

#include "compiler/compiler_extensions.hpp"

namespace kbl
{
	class allocate_checker final
	{
	 public:
		allocate_checker() = default;

		~allocate_checker()
		{
			if (unlikely(armed))
			{
				panic_unchecked();
			}
		}

		void arm(size_t size, bool res)
		{
			if (unlikely(armed))
			{
				panic_doubly_armed();
			}

			armed = true;
			ok = (size == 0 || res);
		}

		bool check()
		{
			armed = false;
			return likely(ok);
		}

	 private:
		bool armed = false;
		bool ok = false;

		[[noreturn]] static void panic_unchecked()
		{
			KDEBUG_GENERALPANIC("allocate_checker::check() isn't called");
		}

		[[noreturn]] static void panic_doubly_armed()
		{
			KDEBUG_GENERALPANIC("allocate_checker::arm() is called twice");
		}
	};
}