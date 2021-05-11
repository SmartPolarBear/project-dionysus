#pragma once

#include "pmm_provider.hpp"
#include "kbl/lock/spinlock.h"

#include "memory/page.hpp"

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
		page* base;
	};

	struct free_area
	{
		list_head freelist;
		size_t free_count;
	};


 public:
	buddy_provider();

	void setup_for_base(page* base, size_t n) override;
	[[nodiscard]] page* allocate(size_t n) override;
	void free(page* base, size_t n) override;
	[[nodiscard]] size_t free_count() const override;
	[[nodiscard]] bool is_well_constructed() const override;

 private:
	bool is_buddy_page(page* page, size_t order, size_t zone);
	page* index_to_page(size_t zone_id, size_t index);
	size_t page_to_index(page* page);
	size_t get_order(size_t n);

	void do_free(page* base, size_t n);
	[[nodiscard]] page* do_allocate(size_t n);

	free_area free_areas_[MAX_ORDER + 1]{};

	zone zones_[ZONE_COUNT_MAX]{};
	size_t zone_count_{ 0 };

	bool well_constructed_{ false };
};

}