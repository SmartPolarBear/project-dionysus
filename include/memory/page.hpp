#pragma once

#include "system/types.h"

enum page_info_flags
{
	PHYSICAL_PAGE_FLAG_RESERVED = 0b01,
	PHYSICAL_PAGE_FLAG_PROPERTY = 0b10,
	PHYSICAL_PAGE_FLAG_ACTIVE = 0b100,
	PHYSICAL_PAGE_FLAG_DIRTY = 0b1000,
	PHYSICAL_PAGE_FLAG_SWAP = 0b1000,
};

// descriptor of physical memory pages
struct page_info
{
	size_t ref;
	size_t flags;
	size_t property;
	size_t zone_id;
	list_head page_link;
};

static inline bool page_has_flag(const page_info* pg, page_info_flags fl)
{
	return pg->flags & fl;
}

static inline void page_set_flag(page_info* pg, page_info_flags fl)
{
	pg->flags |= fl;
}

static inline void page_clear_flag(page_info* pg, page_info_flags fl)
{
	pg->flags &= ~fl;
}
