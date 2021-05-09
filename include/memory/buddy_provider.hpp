#pragma once

#include "pmm_provider.hpp"
#include "kbl/lock/spinlock.h"

namespace memory
{

class buddy_provider final
	: public i_pmm_provider
{
 public:
	static constexpr size_t MAX_ORDER = 12;
	static constexpr size_t ZONE_COUNT_MAX = 8;

	struct zone
	{
		page_info* base;
	};

 public:
	buddy_provider();

	void setup_for_base(page_info* base, size_t n) override;
	[[nodiscard]] page_info* allocate(size_t n) override;
	void free(page_info* base, size_t n) override;
	[[nodiscard]] size_t free_count() const override;
	bool is_well_constructed() const override;

 private:
	bool is_buddy_page(page_info* page, size_t order, size_t zone);
	page_info* index_to_page(size_t zone_id, size_t index);
	size_t page_to_index(page_info* page);
	size_t get_order(size_t n);

	void do_free(page_info* base, size_t n);
	[[nodiscard]] page_info* do_allocate(size_t n);

	free_area_info free_areas_[MAX_ORDER + 1]{};

	zone zones_[ZONE_COUNT_MAX]{};
	size_t zone_count_{ 0 };

	bool well_constructed_{ false };
};

}