#include "memory/buddy_provider.hpp"

void memory::buddy_provider::setup_for_base(page_info* base, size_t n)
{

}

page_info* memory::buddy_provider::allocate(size_t n)
{
	return nullptr;
}

void memory::buddy_provider::free(page_info* base, size_t n)
{

}

size_t memory::buddy_provider::free_count() const
{
	return 0;
}

bool memory::buddy_provider::lock_enable() const
{
	return false;
}

void memory::buddy_provider::set_lock_enable(bool enable)
{

}
