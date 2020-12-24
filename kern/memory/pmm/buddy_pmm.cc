#include "./buddy_pmm.h"

#include "system/memlayout.h"
#include "system/pmm.h"

#include "drivers/debug/kdebug.h"

#include "data/pod_list.h"

#include <cstring>

using libkernel::list_add;
using libkernel::list_add_tail;
using libkernel::list_empty;
using libkernel::list_for_each;
using libkernel::list_init;
using libkernel::list_remove;

using pmm::buddy_pmm::buddy_alloc_pages;
using pmm::buddy_pmm::buddy_free_pages;
using pmm::buddy_pmm::buddy_get_free_pages_count;
using pmm::buddy_pmm::buddy_init_memmap;
using pmm::buddy_pmm::init_buddy;

pmm::pmm_desc pmm::buddy_pmm::buddy_pmm_manager = {
	.name = "BUDDY",
	.init = init_buddy,
	.init_memmap = buddy_init_memmap,
	.alloc_pages = buddy_alloc_pages,
	.free_pages = buddy_free_pages,
	.get_free_pages_count = buddy_get_free_pages_count,
};

constexpr size_t MAX_ORDER = 12;
free_area_info free_areas[MAX_ORDER + 1];

constexpr size_t ZONE_COUNT_MAX = 8;
struct zone_info
{
	page_info* base;
} zones[ZONE_COUNT_MAX] = {{ .base = nullptr }};
size_t zone_count = 0;

static inline size_t get_order(size_t n)
{
	size_t roundup = powerof2_roundup(n);
	return log2(roundup);
}

static inline bool page_is_buddy(page_info* page, size_t order, size_t zone_num)
{
	if (pmm::page_to_index(page) < pmm::page_count)
	{
		if (page->zone_id == zone_num)
		{
			return !page_has_flag(page, PHYSICAL_PAGE_FLAG_RESERVED) &&
				page_has_flag(page, PHYSICAL_PAGE_FLAG_PROPERTY) && page->property == order;
		}
	}
	return false;
}

static inline size_t buddy_page_to_index(page_info* page)
{
	return page - zones[page->zone_id].base;
}

static inline page_info* index_to_page(size_t zone_id, size_t index)
{
	return zones[zone_id].base + index;
}

static inline void buddy_free_pages_impl(page_info* base, size_t order)
{
	size_t buddy_index = 0, page_index = buddy_page_to_index(base);

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
		if (!page_is_buddy(buddy, order, zone_id))
		{
			break;
		}

		free_areas[order].free_count--;

		list_remove(&buddy->page_link);
		page_clear_flag(buddy, PHYSICAL_PAGE_FLAG_PROPERTY);

		page_index &= buddy_index;
		order++;
	}

	auto page = index_to_page(zone_id, page_index);

	page->property = order;
	page_set_flag(page, PHYSICAL_PAGE_FLAG_PROPERTY);
	free_areas[order].free_count++;
	list_add(&page->page_link, &free_areas[order].freelist);
}

static inline page_info* buddy_alloc_pages_impl(size_t order)
{
	KDEBUG_ASSERT(order <= MAX_ORDER);

	for (size_t cur_order = order; cur_order <= MAX_ORDER; cur_order++)
	{
		if (!list_empty(&free_areas[cur_order].freelist))
		{

			auto entry = free_areas[cur_order].freelist.next;
			page_info* page = list_entry(entry, page_info, page_link);

			free_areas[cur_order].free_count--;

			list_remove(entry);

			size_t sz = 1 << cur_order;

			while (cur_order > order)
			{
				cur_order--;
				sz >>= 1;

				page_info* buddy = page + sz;
				buddy->property = cur_order;

				page_set_flag(buddy, PHYSICAL_PAGE_FLAG_PROPERTY);

				free_areas[cur_order].free_count++;

				list_add(&buddy->page_link, &free_areas[cur_order].freelist);
			}

			page_clear_flag(page, PHYSICAL_PAGE_FLAG_PROPERTY);
			return page;
		}
	}

	return nullptr;
}

void pmm::buddy_pmm::init_buddy(void)
{
	for (size_t i = 0; i <= MAX_ORDER; i++)
	{
		list_init(&free_areas[i].freelist);
		free_areas[i].free_count = 0;
	}
}

void pmm::buddy_pmm::buddy_init_memmap(page_info* base, size_t n)
{
	KDEBUG_ASSERT(n > 0);
	KDEBUG_ASSERT(zone_count < ZONE_COUNT_MAX);

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
		p->zone_id = zone_count;
	}

	zones[zone_count].base = base;
	zone_count++;

	size_t order = MAX_ORDER, order_size = (1 << order);

	for (page_info* p = base + (n - 1); n != 0;)
	{
		while (n >= order_size)
		{
			page_set_flag(p, PHYSICAL_PAGE_FLAG_PROPERTY);
			p->property = order;

			list_add(&p->page_link, &free_areas[order].freelist);

			n -= order_size;
			p -= order_size;

			free_areas[order].free_count++;
		}

		order--;
		order_size >>= 1;
	}
}

page_info* pmm::buddy_pmm::buddy_alloc_pages(size_t n)
{
	KDEBUG_ASSERT(n > 0);

	size_t order = get_order(n), order_size = (1 << order);

	page_info* page = buddy_alloc_pages_impl(order);
	if (page != nullptr && n != order_size)
	{
		pmm::free_pages(page + n, order_size - n);
	}

	return page;
}

void pmm::buddy_pmm::buddy_free_pages(page_info* base, size_t n)
{
	KDEBUG_ASSERT(n > 0);

	if (n == 1)
	{
		buddy_free_pages_impl(base, 0);
	}
	else
	{
		size_t order = 0, order_size = 1;
		while (n >= order_size)
		{
			KDEBUG_ASSERT(order <= MAX_ORDER);
			if ((buddy_page_to_index(base) & order_size) != 0)
			{
				buddy_free_pages_impl(base, order);
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
			buddy_free_pages_impl(base, order);
			base += order_size;
			n -= order_size;
		}
	}
}

size_t pmm::buddy_pmm::buddy_get_free_pages_count(void)
{
	size_t ret = 0;
	for (size_t order = 0; order <= MAX_ORDER; order++)
	{
		ret += free_areas[order].free_count * (1 << order);
	}
	return ret;
}
