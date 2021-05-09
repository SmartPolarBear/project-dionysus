#pragma once

#include "pmm_provider.hpp"
#include "kbl/lock/spinlock.h"

namespace memory
{

class buddy_provider final
	: public i_pmm_provider
{
 public:
	void setup_for_base(page_info* base, size_t n) override;
	[[nodiscard]] page_info* allocate(size_t n) override;
	void free(page_info* base, size_t n) override;
	[[nodiscard]] size_t free_count() const override;
	[[nodiscard]] bool lock_enable() const override;
	void set_lock_enable(bool enable) override;

 private:
	lock::spinlock lock_;
};

}