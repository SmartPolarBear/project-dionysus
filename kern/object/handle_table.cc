#include "object/handle_table.hpp"

#include "task/process/process.hpp"

using namespace object;

template<std::convertible_to<dispatcher> T>
error_code_with_result<std::shared_ptr<T>> handle_table::object_from_handle(const handle_entry& h)
{
	return downcast_dispatcher<T>(h.ptr_);
}

void handle_table::add_handle(const handle_entry& h)
{

}

void handle_table::remove_handle(handle_entry& h)
{

}

bool handle_table::valid_handle(const handle_entry& h) const
{
	return false;
}
