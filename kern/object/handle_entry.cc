#include "system/types.h"
#include "system/kom.hpp"

#include "object/handle_entry.hpp"
#include "object/handle_table.hpp"

using namespace object;
using namespace lock;

handle_entry_owner object::handle_entry::make(const dispatcher* obj, bool is_global)
{
	obj->adopt();
	kbl::allocate_checker ck;
	auto ret = new(&ck) handle_entry(obj);

	if (!ck.check())
	{
		return nullptr;
	}

	if (is_global)
	{
		lock_guard g{ handle_table::global_lock_ };
		obj->add_ref();
		handle_table::global_handles_.push_back(ret);
	}

	return handle_entry_owner(ret);
}

object::handle_entry_owner object::handle_entry::duplicate(handle_entry* h)
{
	h->canary_.assert();
	h->ptr_->add_ref();

	kbl::allocate_checker ck;
	auto ret = new(&ck) handle_entry(*h);

	if (!ck.check())
	{
		return nullptr;
	}

	return handle_entry_owner(ret);
}

void object::handle_entry::release(handle_entry_owner h)
{
	h->canary_.assert();
	if (h->ptr_->release())
	{
		delete h->ptr_;
	}
}

void handle_entry_deleter::operator()(handle_entry* e)
{
	lock_guard g{ handle_table::global_lock_ };

	handle_table::global_handles_.remove(e);

	delete e;
}
