#include "system/types.h"
#include "system/kom.hpp"

#include "object/handle.hpp"
#include "object/handle_table.hpp"

using namespace object;

handle object::handle::make(std::convertible_to<ref_counted> auto& obj, object_type t)
{
	auto& rc = static_cast<ref_counted&>(obj);
	rc.adopt();
	return handle(rc, t);
}

object::handle object::handle::duplicate(const object::handle& h)
{
	h.obj_.add_ref();
	handle copy{ h };
	return copy;
}

void object::handle::release(const object::handle& h)
{
	h.obj_.release();
}

