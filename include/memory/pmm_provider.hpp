#pragma once

#include "memory/page.hpp"

namespace memory
{
class i_pmm_provider
{
 public:
	virtual void setup_for_base(page* base, size_t n) = 0;

	[[nodiscard]] virtual page* allocate(size_t n) = 0;

	virtual void free(page* base, size_t n) = 0;

	[[nodiscard]] virtual bool is_well_constructed() const = 0;

	[[nodiscard]] virtual size_t free_count() const = 0;
};
}