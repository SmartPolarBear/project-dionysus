#pragma once

#include "system/types.h"

namespace object
{
	using kernel_object_id_type = int64_t;
	class kernel_object
	{
	 public:
		virtual ~kernel_object() = default;

		[[nodiscard]] kernel_object_id_type get_koid() const
		{
			return koid;
		}
		void set_koid(kernel_object_id_type _koid)
		{
			kernel_object::koid = _koid;
		}

	 private:
		kernel_object_id_type koid{};

	};
}