#include "memory/buddy_provider.hpp"

#include "system/memlayout.h"
#include "system/mmu.h"
#include "system/pmm.h"

#include "debug/kdebug.h"

using namespace kbl;

size_t memory::buddy_provider::get_order(size_t n)
{
	size_t roundup = powerof2_roundup(n);
	return log2(roundup);
}

void memory::buddy_provider::setup_for_base(page_info* base, size_t n)
{
	KDEBUG_ASSERT(n > 0);
	KDEBUG_ASSERT(zone_count_ < ZONE_COUNT_MAX);

	for (page_info* p = base; p != base + n; p++)
	{
		// KDEBUG_ASSERT(page_has_flag(p, PHYSICAL_PAGE_FLAG_RESERVED));
		if (!page_has_flag(p, PHYSICAL_PAGE_FLAG_RESERVED))
		{
			KDEBUG_RICHPANIC("page_has_flag(p, PHYSICAL_PAGE_FLAG_RESERVED) should be 1\n", "ASSERTION", true,
				"Page index=%d\n", pmm::page_to_index(p));
		}
		p->flags = 0;
		p->property = 0;
		p->ref = 0;
		p->zone_id = zone_count_;
	}

	zones_[zone_count_].base = base;
	zone_count_++;

	size_t order = MAX_ORDER, order_size = (1 << order);

	for (page_info* p = base + (n - 1); n != 0;)
	{
		while (n >= order_size)
		{
			page_set_flag(p, PHYSICAL_PAGE_FLAG_PROPERTY);
			p->property = order;

			list_add(&p->page_link, &free_areas_[order].freelist);

			n -= order_size;
			p -= order_size;

			free_areas_[order].free_count++;
		}

		order--;
		order_size >>= 1;
	}
}

page_info* memory::buddy_provider::allocate(size_t n)
{
	KDEBUG_ASSERT(n > 0);

	size_t order = get_order(n), order_size = (1 << order);

	page_info* page = do_allocate(order);
	if (page != nullptr && n != order_size)
	{
//		pmm::free_pages(page + n, order_size - n);
//		pmm::buddy_pmm::buddy_free_pages(page + n, order_size - n);
		free(page + n, order_size - n);
	}

	return page;
}

void memory::buddy_provider::free(page_info* base, size_t n)
{
	KDEBUG_ASSERT(n > 0);

	if (n == 1)
	{
		do_free(base, 0);
	}
	else
	{
		size_t order = 0, order_size = 1;
		while (n >= order_size)
		{
			KDEBUG_ASSERT(order <= MAX_ORDER);
			if ((page_to_index(base) & order_size) != 0)
			{
				do_free(base, order);
				base += order_size;
				n -= order_size;
			}
			order++;
			order_size <<= 1;
		}

		while (n != 0)
		{
			while (n < order_size)
			{
				order--;
				order_size >>= 1;
			}
			do_free(base, order);
			base += order_size;
			n -= order_size;
		}
	}
}

size_t memory::buddy_provider::free_count() const
{
	size_t ret = 0;
	for (size_t order = 0; order <= MAX_ORDER; order++)
	{
		ret += free_areas_[order].free_count * (1 << order);
	}
	return ret;
}

bool memory::buddy_provider::is_buddy_page(page_info* page, size_t order, size_t zone)
{

	if (pmm::page_to_index(page) < pmm::page_count)
	{
		if (page->zone_id == zone)
		{
			return !page_has_flag(page, PHYSICAL_PAGE_FLAG_RESERVED) &&
				page_has_flag(page, PHYSICAL_PAGE_FLAG_PROPERTY) && page->property == order;
		}
	}
	return false;

}

page_info* memory::buddy_provider::index_to_page(size_t zone_id, size_t index)
{
	return zones_[zone_id].base + index;
}

size_t memory::buddy_provider::page_to_index(page_info* page)
{
	return page - zones_[page->zone_id].base;
}

void memory::buddy_provider::do_free(page_info* base, size_t order)
{
	size_t buddy_index = 0, page_index = page_to_index(base);

	KDEBUG_ASSERT((page_index & ((1 << order) - 1)) == 0);

	for (page_info* p = base; p != base + (1 << order); p++)
	{
		KDEBUG_ASSERT(!page_has_flag(p, PHYSICAL_PAGE_FLAG_RESERVED));
		KDEBUG_ASSERT(!page_has_flag(p, PHYSICAL_PAGE_FLAG_PROPERTY));

		p->flags = 0;
		p->ref = 0;
	}

	size_t zone_id = base->zone_id;
	while (order < MAX_ORDER)
	{
		buddy_index = page_index ^ (1 << order);

		auto buddy = index_to_page(zone_id, buddy_index);
		if (!is_buddy_page(buddy, order, zone_id))
		{
			break;
		}

		free_areas_[order].free_count--;

		list_remove(&buddy->page_link);
		page_clear_flag(buddy, PHYSICAL_PAGE_FLAG_PROPERTY);

		page_index &= buddy_index;
		order++;
	}

	auto page = index_to_page(zone_id, page_index);

	page->property = order;
	page_set_flag(page, PHYSICAL_PAGE_FLAG_PROPERTY);
	free_areas_[order].free_count++;
	list_add(&page->page_link, &free_areas_[order].freelist);
}

page_info* memory::buddy_provider::do_allocate(size_t order)
{
	KDEBUG_ASSERT(order <= MAX_ORDER);

	for (size_t cur_order = order; cur_order <= MAX_ORDER; cur_order++)
	{
		if (!list_empty(&free_areas_[cur_order].freelist))
		{

			auto entry = free_areas_[cur_order].freelist.next;
			page_info* page = list_entry(entry, page_info, page_link);

			free_areas_[cur_order].free_count--;

			list_remove(entry);

			size_t sz = 1 << cur_order;

			while (cur_order > order)
			{
				cur_order--;
				sz >>= 1;

				page_info* buddy = page + sz;
				buddy->property = cur_order;

				page_set_flag(buddy, PHYSICAL_PAGE_FLAG_PROPERTY);

				free_areas_[cur_order].free_count++;

				list_add(&buddy->page_link, &free_areas_[cur_order].freelist);
			}

			page_clear_flag(page, PHYSICAL_PAGE_FLAG_PROPERTY);
			return page;
		}
	}

	return nullptr;
}

memory::buddy_provider::buddy_provider()
{
	for (size_t i = 0; i <= MAX_ORDER; i++)
	{
		list_init(&free_areas_[i].freelist);
		free_areas_[i].free_count = 0;
	}

	well_constructed_ = true;
}

bool memory::buddy_provider::is_well_constructed() const
{
	return well_constructed_;
}
