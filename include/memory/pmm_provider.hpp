#pragma once

#include "system/memlayout.h"

namespace memory
{
class i_pmm_provider
{
 public:
	virtual void setup_for_base(page_info* base, size_t n) = 0;

	virtual page_info* allocate(size_t n) = 0;

	virtual void free(page_info* base, size_t n) = 0;

	[[nodiscard]] virtual size_t free_count() const = 0;
};
}