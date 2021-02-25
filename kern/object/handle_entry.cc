#include "system/types.h"
#include "system/kom.hpp"

#include "object/handle_entry.hpp"
#include "object/handle_table.hpp"

using namespace object;
using namespace lock;

handle_entry_owner object::handle_entry::create(ktl::string_view name, const dispatcher* obj)
{
	if (obj->adopted())
	{
		return nullptr;
	}

	obj->adopt();

	kbl::allocate_checker ck;
	auto ret = new(&ck) handle_entry(obj, name);

	if (!ck.check())
	{
		return nullptr;
	}

	return handle_entry_owner(ret);
}

object::handle_entry_owner object::handle_entry::duplicate(handle_entry* h)
{
	h->canary_.assert();

	if (!h->ptr_->adopted())
	{
		return nullptr;
	}

	kbl::allocate_checker ck;
	auto ret = new(&ck) handle_entry(*h); // use copy constructor

	if (!ck.check())
	{
		return nullptr;
	}

	return handle_entry_owner(ret);
}

void handle_entry::release(handle_entry* h)
{
	delete h;
}

void object::handle_entry::release(handle_entry_owner h)
{
	h->canary_.assert();
	return release(h.release());
}

void handle_entry_deleter::operator()(handle_entry* e)
{
	e->canary_.assert();
	handle_entry::release(e);
}
