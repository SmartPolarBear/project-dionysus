#pragma once

#include "memory/fpage.hpp"

#include "system/memlayout.h"

#include "kbl/singleton/singleton.hpp"

#include "memory/pmm_provider.hpp"

#include "memory/buddy_provider.hpp"

namespace memory
{

class physical_memory_manager final
	// we do not use singleton because it overwrite the ap_boot
	//	: public kbl::singleton<physical_memory_manager>
{
 public:
	using provider_type = buddy_provider;

	physical_memory_manager(const physical_memory_manager&) = delete;
	physical_memory_manager& operator=(const physical_memory_manager&) = delete;

	static physical_memory_manager* instance();
 public:

	physical_memory_manager();

	void setup_for_base(page* base, size_t n);
	[[nodiscard]] page* allocate(size_t n);

	void free(page* base);
	void free(page* base, size_t n);

	[[nodiscard]] size_t free_count() const;

	[[nodiscard]] bool is_well_constructed() const;

 private:
	provider_type provider_{};
	lock::spinlock lock_{ "pmm" };
};

}