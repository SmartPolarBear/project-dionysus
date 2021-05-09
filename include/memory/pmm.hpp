#pragma once

#include "memory/fpage.hpp"

#include "system/memlayout.h"

#include "kbl/singleton/singleton.hpp"

#include "memory/pmm_provider.hpp"

#include "memory/buddy_provider.hpp"

namespace memory
{

class physical_memory_manager final
	: public kbl::singleton<physical_memory_manager>
{
 public:
	using provider_type = buddy_provider;

 public:

	physical_memory_manager();

 private:
	provider_type provider_{};
};

}