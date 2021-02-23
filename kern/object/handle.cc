#include "system/types.h"
#include "system/kom.hpp"

#include "object/handle.hpp"
#include "object/handle_table.hpp"

using namespace object;

handle object::handle::make(std::move_constructible auto& obj)
{
	return handle();
}

object::handle object::handle::duplicate(const object::handle& h)
{
	return object::handle();
}

object::handle object::handle::release(const object::handle& h)
{
	return object::handle();
}

