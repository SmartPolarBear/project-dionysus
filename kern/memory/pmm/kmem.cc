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

#include "system/kmem.hpp"
#include "system/pmm.h"

#include "drivers/console/console.h"
#include "debug/kdebug.h"

#include "memory/pmm.hpp"

#include "kbl/lock/lock_guard.hpp"

#include <cstring>

using namespace memory;
using namespace memory::kmem;

// linked list
using namespace kbl;

using namespace lock;

struct slab
{
	size_t ref{};
	kmem_cache* cache{};
	size_t inuse{}, next_free{};
	void* obj_ptr{};
	kmem_bufctl* freelist{};

	lock::spinlock lock{ "slab" };

	list_head slab_link{};
};

list_head cache_head;
spinlock cache_head_lock{ "kmem_cache_head" };

kmem_cache* sized_caches[KMEM_SIZED_CACHE_COUNT];
kmem_cache cache_cache;

static inline constexpr size_t cache_obj_count(kmem_cache* cache, size_t obj_size)
{
	// return PMM_PAGE_SIZE / (sizeof(kmem_bufctl) + obj_size);
	size_t header_size = (sizeof(kmem_bufctl) + obj_size);
	if (cache->flags & memory::kmem::KMEM_CACHE_4KALIGN)
	{
		header_size = roundup(header_size, (uintptr_t)4_KB);
	}

	return PAGE_SIZE / header_size;
}

static inline void* slab_cache_grow(kmem_cache* cache)
{
	cache->lock.assert_held();

	auto page = memory::physical_memory_manager::instance()->allocate(); //alloc_page();

	auto block = (void*)pmm::page_to_va(page);

	if (block == nullptr)
	{
		return block;
	}

	auto slb = new(block) slab{};  //reinterpret_cast<decltype(slb)>(block);
	lock::lock_guard g{ slb->lock };

	slb->cache = cache;
	slb->next_free = 0;
	slb->inuse = 0;

	list_add(&slb->slab_link, &cache->free);

	slb->freelist = reinterpret_cast<decltype(slb->freelist)>(((char*)block) + sizeof(slab));

	uintptr_t
		after_freelist_addr = (uintptr_t)(((char*)block) + sizeof(slab) + sizeof(kmem_bufctl) * cache->obj_count + 16);
	if (cache->flags & memory::kmem::KMEM_CACHE_4KALIGN)
	{
		after_freelist_addr = roundup(after_freelist_addr, (uintptr_t)4_KB);
	}

	slb->obj_ptr = reinterpret_cast<decltype(slb->obj_ptr)>(after_freelist_addr);

	void* obj = slb->obj_ptr;
	for (size_t i = 0; i < cache->obj_count; i++)
	{
		if (cache->ctor)
		{
			cache->ctor(obj, cache, cache->obj_size);
		}

		obj = (void*)((char*)obj + cache->obj_size);
		slb->freelist[i] = i + 1;
	}
	slb->freelist[cache->obj_count - 1] = -1;

	return slb;
}

static inline void slab_destory(kmem_cache* cache, slab* slb)
{
	// Precondition: the cache's lock must be held
	cache->lock.assert_held();

	lock_guard g{ slb->lock };

	void* obj = slb->obj_ptr;
	for (size_t i = 0; i < cache->obj_count; i++)
	{
		if (cache->dtor)
		{
			cache->dtor(obj, cache, cache->obj_size);
		}
	}

	list_remove(&slb->slab_link);

	uintptr_t va = (uintptr_t)slb;

	physical_memory_manager::instance()->free((page*)(pmm::va_to_page(va)));
//	free_page((page*)(pmm::va_to_page(va)));
}

// ATTENTION: BE CAUTIOUS FOR RACE CONDITION
// This function should not be a critial section because there's no modifying
static inline slab* slab_find(kmem_cache* cache, void* obj)
{
	list_head* heads[] = { &cache->partial, &cache->full };
	slab* slb = nullptr;

	for (auto head : heads)
	{
		list_head* iter = nullptr;
		list_for(iter, head)
		{
			slab* s = list_entry(iter, slab, slab_link);
			uintptr_t begin = (uintptr_t)s->obj_ptr;
			uintptr_t end = (uintptr_t)(((char*)s->obj_ptr) + cache->obj_size * cache->obj_count);
			if (((uintptr_t)obj) >= begin && ((uintptr_t)obj) < end)
			{
				slb = s;
				break;
			}
		}
	}

	return slb;
}

void memory::kmem::kmem_init()
{
	cache_cache.obj_size = sizeof(decltype(cache_cache));
	cache_cache.obj_count = cache_obj_count(&cache_cache, cache_cache.obj_size);
	cache_cache.ctor = nullptr;
	cache_cache.dtor = nullptr;

	auto cache_cache_name = "cache_cache";
	strncpy(cache_cache.name, cache_cache_name, KMEM_CACHE_NAME_MAXLEN);

	list_init(&cache_cache.full);
	list_init(&cache_cache.partial);
	list_init(&cache_cache.free);

	list_init(&cache_head);
	list_add(&cache_cache.cache_link, &cache_head);

	char sized_cache_name[KMEM_CACHE_NAME_MAXLEN];
	size_t sized_cache_count = 0;
	for (size_t sz = KMEM_MIN_SIZED_CACHE_SIZE; sz <= KMEM_MAX_SIZED_CACHE_SIZE; sz *= 2)
	{

		memset(sized_cache_name, 0, sizeof(sized_cache_name));

		sized_cache_name[0] = 's';
		sized_cache_name[1] = 'i';
		sized_cache_name[2] = 'z';
		sized_cache_name[3] = 'e';
		sized_cache_name[4] = '_';
		sized_cache_name[5] = '0' + char(sz);

		sized_caches[sized_cache_count++] = kmem_cache_create(sized_cache_name, sz, nullptr, nullptr);
	}

	KDEBUG_ASSERT(sized_cache_count == sized_cache_count);
}

kmem_cache* memory::kmem::kmem_cache_create(const char* name,
	size_t size,
	kmem_ctor_type ctor,
	kmem_dtor_type dtor,
	size_t flags)
{
	KDEBUG_ASSERT(size < PAGE_SIZE - sizeof(kmem_bufctl));
	kmem_cache* ret =
		new(kmem_cache_alloc(&cache_cache))kmem_cache{}; //reinterpret_cast<decltype(ret)>(kmem_cache_alloc(&cache_cache));

	if (ret != nullptr)
	{
		ret->obj_size = size;
		ret->obj_count = cache_obj_count(ret, size);

		ret->ctor = ctor;
		ret->dtor = dtor;

		ret->flags = flags;

		strncpy(ret->name, name, KMEM_CACHE_NAME_MAXLEN);

		list_init(&ret->full);
		list_init(&ret->partial);
		list_init(&ret->free);

		{
			lock_guard gcache{ cache_head_lock };

			list_add(&ret->cache_link, &cache_head);
		}

	}
	else
	{
		KDEBUG_GENERALPANIC("Insufficient memory for slab initialization.");
	}

	return ret;
}

void* memory::kmem::kmem_cache_alloc(kmem_cache* cache)
{
	lock_guard g1{ cache->lock };

	list_head* entry = nullptr;
	if (!list_empty(&cache->partial))
	{
		entry = cache->partial.next;
	}
	else
	{
		if (list_empty(&cache->free) && slab_cache_grow(cache) == nullptr)
		{
			return nullptr;
		}

		entry = cache->free.next;
	}

	KDEBUG_ASSERT(entry != nullptr);

	list_remove(entry);
	slab* slb = list_entry(entry, slab, slab_link);
	lock_guard g{ slb->lock };

	void* ret = (void*)(((uint8_t*)slb->obj_ptr) + slb->next_free * cache->obj_size);

	slb->inuse++;
	slb->next_free = slb->freelist[slb->next_free];

	if (slb->inuse == cache->obj_count)
	{
		list_add(entry, &cache->full);
	}
	else
	{
		list_add(entry, &cache->partial);
	}

	if (cache->flags & KMEM_CACHE_4KALIGN)
	{
		KDEBUG_ASSERT((((uintptr_t)ret) % 4_KB) == 0);
	}

	return ret;
}

void memory::kmem::kmem_cache_destroy(kmem_cache* cache)
{
	{
		lock_guard g1{ cache->lock };

		list_head* heads[] = { &cache->full, &cache->partial, &cache->free };
		for (auto head : heads)
		{
			auto entry = head->next;
			while (head != entry)
			{
				auto slb = list_entry(entry, slab, slab_link);
				entry = entry->next;
				slab_destory(cache, slb);
			}
		}
	}

	kmem_cache_free(&cache_cache, cache);
}

void memory::kmem::kmem_cache_free(kmem_cache* cache, void* obj)
{
	KDEBUG_ASSERT(obj != nullptr && cache != nullptr);

	slab* slb = slab_find(cache, obj);
	KDEBUG_ASSERT(slb != nullptr);
	lock_guard g{ slb->lock };

	size_t offset = (((uintptr_t)obj) - ((uintptr_t)slb->obj_ptr)) / cache->obj_size;

	lock_guard g1{ cache->lock };

	list_remove(&slb->slab_link);
	slb->freelist[offset] = slb->next_free;
	slb->next_free = offset;
	slb->inuse--;

	if (slb->inuse == 0)
	{
		list_add(&slb->slab_link, &cache->free);
	}
	else
	{
		list_add(&slb->slab_link, &cache->partial);
	}

}

size_t memory::kmem::kmem_cache_shrink(kmem_cache* cache)
{
	lock_guard g1{ cache->lock };

	size_t count = 0;
	auto entry = cache->free.next;
	auto head = &cache->free;

	while (head != entry)
	{
		auto slb = list_entry(entry, slab, slab_link);
		entry = entry->next;
		slab_destory(cache, slb);
		count++;
	}

	return count;
}

size_t memory::kmem::kmem_cache_reap()
{
	size_t count = 0;
	list_head* iter = nullptr;
	list_for(iter, &cache_head)
	{
		count += kmem_cache_shrink(list_entry(iter, kmem_cache, cache_link));
	}
	return count;
}
