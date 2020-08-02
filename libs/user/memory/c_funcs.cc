#include "dionysus.hpp"

[[maybe_unused]] void *malloc(size_t size)
{
	return heap_alloc(size,0);
}

[[maybe_unused]] void free(void *ptr)
{
	heap_free(ptr);
}