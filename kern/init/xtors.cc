/// \brief C++ ctors and dtors. They are called by boot.S
#include "system/kernel_layout.hpp"

extern "C" void call_ctors(void)
{
	for (auto ctor = &start_ctors; ctor != &end_ctors; ctor++)
	{
		(*ctor)();
	}
}

extern "C" void call_dtors(void)
{
	for (auto dtor = &start_dtors; dtor != &end_dtors; dtor++)
	{
		(*dtor)();
	}
}

