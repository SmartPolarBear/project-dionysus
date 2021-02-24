#include "system/types.h"
#include "system/kom.hpp"

#include "object/handle_entry.hpp"
#include "object/handle_table.hpp"

using namespace object;

kbl::intrusive_list<handle_entry,
                    lock::spinlock,
                    handle_entry::node_trait,
                    true> object::handle_entry::global_list{};

handle_entry object::handle_entry::make(const ktl::shared_ptr<dispatcher>& obj, object_type t)
{
	obj->adopt();
	kbl::allocate_checker ck;
	auto ret = new(&ck) handle_entry(obj, t);

	if (!ck.check())
	{

	}

	global_list.push_back(ret);
	return handle_entry_owner(ret);
}

object::handle_entry object::handle_entry::duplicate(const object::handle_entry& h)
{
	h.ptr_->add_ref();
	handle_entry copy{ h };
	return copy;
}

void object::handle_entry::release(const object::handle_entry& h)
{
	if (h.ptr_->release())
	{

	}
}

void handle_entry_deleter::operator()(handle_entry* e)
{

}
