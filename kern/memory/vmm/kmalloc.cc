

// Copyright (c) 2021 SmartPolarBear
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "system/kmalloc.hpp"
#include "system/kmem.hpp"
#include "system/pmm.h"

#include "memory/pmm.hpp"

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

	// assumption: sized_caches are sorted ascending.
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

		ret = reinterpret_cast<decltype(ret)>(pmm::page_to_va(physical_memory_manager::instance()->allocate(npages)));

		ret->type = allocator_types::PMM;
		ret->alloc_info.pmm.page_count = npages;
	}
	else
	{
		// use slab
		auto cache = cache_from_size(actual_size);

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
		physical_memory_manager::instance()->free(pmm::va_to_page((uintptr_t)ptr), block->alloc_info.pmm.page_count);

	}
	else if (block->type == allocator_types::SLAB)
	{
		auto cache = block->alloc_info.slab.cache;
		kmem_cache_free(cache, block);
	}
}