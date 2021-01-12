#pragma once

#include "system/types.h"

namespace object
{

using koid_type = int64_t;

class object
{
 public:
	virtual ~object() = default;

	[[nodiscard]] koid_type get_koid() const
	{
		return koid;
	}

	void set_koid(koid_type _koid)
	{
		koid = _koid;
	}

 private:
	koid_type koid{ 0 };

};

}