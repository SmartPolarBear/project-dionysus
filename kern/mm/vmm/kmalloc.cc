/*
 * Last Modified: Sun May 10 2020
 * Modified By: SmartPolarBear
 * -----
 * Copyright (C) 2006 by SmartPolarBear <clevercoolbear@outlook.com>
 * 
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 * -----
 * HISTORY:
 * Date      	By	Comments
 * ----------	---	----------------------------------------------------------
 */

#include "system/kmalloc.h"
#include "system/kmem.h"
#include "system/pmm.h"

// slab
using memory::kmem::kmem_bufctl;
using memory::kmem::kmem_cache;
using memory::kmem::KMEM_CACHE_NAME_MAXLEN;
using memory::kmem::KMEM_SIZED_CACHE_COUNT;

using memory::kmem::kmem_cache_alloc;
using memory::kmem::kmem_cache_create;
using memory::kmem::kmem_cache_destroy;
using memory::kmem::kmem_cache_free;
using memory::kmem::kmem_cache_reap;
using memory::kmem::kmem_cache_shrink;
using memory::kmem::kmem_init;

// defined in slab.cc
extern kmem_cache* sized_caches[KMEM_SIZED_CACHE_COUNT];

// pmm
using pmm::alloc_page;
using pmm::alloc_pages;
using pmm::free_page;
using pmm::free_pages;

constexpr size_t GUARDIAN_BYTES_AFTER = 16;

enum class allocator_types
{
	// not start from zero for the sake of debugging
	PMM = 0x1,
	SLAB
};

struct memory_block
{
	allocator_types type;

	union
	{
		struct
		{
			size_t page_count;
		} pmm;

		struct
		{
			size_t size;
			kmem_cache* cache;
		} slab;
	} alloc_info;

	uint8_t mem[0];
};

static inline kmem_cache* cache_from_size(size_t sz)
{
	kmem_cache* cache = nullptr;

	// assumption: sized_caches are sorted ascendingly.
	for (auto c : sized_caches)
	{
		if (sz <= c->obj_size)
		{
			cache = c;
			break;
		}
	}

	return cache;
}

void* memory::kmalloc(size_t sz, [[maybe_unused]] size_t flags)
{
	memory_block* ret = nullptr;

	size_t actual_size = sizeof(memory_block) + sz + GUARDIAN_BYTES_AFTER;

	if (actual_size > memory::kmem::KMEM_MAX_SIZED_CACHE_SIZE)
	{
		// use buddy

		size_t npages = roundup(actual_size, PAGE_SIZE) / PAGE_SIZE;

		ret = reinterpret_cast<decltype(ret)>(pmm::page_to_va(pmm::alloc_pages(npages)));

		ret->type = allocator_types::PMM;
		ret->alloc_info.pmm.page_count = npages;
	}
	else
	{
		// use slab
		kmem_cache* cache = cache_from_size(actual_size);

		ret = reinterpret_cast<decltype(ret)>(kmem_cache_alloc(cache));
		ret->type = allocator_types::SLAB;
		ret->alloc_info.slab = decltype(ret->alloc_info.slab){ .size = cache->obj_size, .cache = cache };
	}

	return ret->mem;
}

void memory::kfree(void* ptr)
{
	uint8_t* mem_ptr = reinterpret_cast<decltype(mem_ptr)>(ptr);
	memory_block* block = reinterpret_cast<decltype(block)>(container_of(mem_ptr, memory_block, mem));

	if (block->type == allocator_types::PMM)
	{
		pmm::free_pages(pmm::va_to_page((uintptr_t)ptr), block->alloc_info.pmm.page_count);
	}
	else if (block->type == allocator_types::SLAB)
	{
		auto cache = block->alloc_info.slab.cache;
		kmem_cache_free(cache, block);
	}
}