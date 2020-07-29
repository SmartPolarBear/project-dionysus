#include "dionysus.hpp"

[[maybe_unused]] void *malloc(size_t size)
{
	return malloc(size);
}

[[maybe_unused]] void free(void *ptr)
{
	free(ptr);
}