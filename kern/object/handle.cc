#include "system/types.h"
#include "system/kom.hpp"

#include "object/handle.hpp"
#include "object/handle_table.hpp"

using namespace object;

handle object::handle::make(ktl::shared_ptr<dispatcher> obj, object_type t)
{
	obj->adopt();
	return handle(obj, t);
}

object::handle object::handle::duplicate(const object::handle& h)
{
	h.obj_->add_ref();
	handle copy{ h };
	return copy;
}

void object::handle::release(const object::handle& h)
{
	if (h.obj_->release())
	{

	}
}

