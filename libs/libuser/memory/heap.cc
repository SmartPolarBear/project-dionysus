#include "dionysus.hpp"

// Memory allocator by Kernighan and Ritchie,
// The C programming Language, 2nd ed.  Section 8.7.

struct alignas(sizeof(uint64_t)) Header
{
	Header* ptr;
	size_t size;
};

static_assert(sizeof(Header) % sizeof(uint64_t) == 0);

static Header base;
static Header* freep;

static inline Header* heap_expand(size_t nu)
{
	uintptr_t heap_ptr = 0;

	if (set_heap(&heap_ptr) != ERROR_SUCCESS)
	{
		return nullptr;
	}

	uintptr_t new_heap_ptr = heap_ptr + nu * sizeof(Header);
	if (set_heap(&new_heap_ptr) != ERROR_SUCCESS || new_heap_ptr < heap_ptr)
	{
		return nullptr;
	}

	auto hp = (Header*)new_heap_ptr;
	hp->size = nu;

	heap_free((void*)(hp + 1));

	return freep;
}

void heap_free(void* ap)
{

	auto bp = (Header*)ap - 1, p = freep;
	for (p = freep; !(bp > p && bp < p->ptr); p = p->ptr)
		if (p >= p->ptr && (bp > p || bp < p->ptr))
			break;

	if (bp + bp->size == p->ptr)
	{
		bp->size += p->ptr->size;
		bp->ptr = p->ptr->ptr;
	}
	else
		bp->ptr = p->ptr;
	if (p + p->size == bp)
	{
		p->size += bp->size;
		p->ptr = bp->ptr;
	}
	else
		p->ptr = bp;
	freep = p;
}

void* heap_alloc(size_t size, [[maybe_unused]]uint64_t flags)
{
	Header* prevp = freep;

	size_t nunits = (size + sizeof(Header) - 1) / sizeof(Header) + 1;

	if (prevp == nullptr)
	{
		base.ptr = freep = prevp = &base;
		base.size = 0;
	}


	for (auto p = prevp->ptr;; prevp = p, p = p->ptr)
	{
		if (p->size >= nunits)
		{
			if (p->size == nunits)
				prevp->ptr = p->ptr;
			else
			{
				p->size -= nunits;
				p += p->size;
				p->size = nunits;
			}
			freep = prevp;
			return (void*)(p + 1);
		}
		if (p == freep)
		{
			if ((p = heap_expand(nunits)) == nullptr)
			{
				return nullptr;
			}
		}
	}
}
