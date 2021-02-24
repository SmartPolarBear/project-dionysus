#include "object/handle_table.hpp"

#include "task/process/process.hpp"

using namespace object;

template<>
error_code_with_result<task::process> handle_table::object_from_handle(handle h)
{
	return h.obj_.get();
}