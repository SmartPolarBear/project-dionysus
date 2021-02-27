// C++ ctors and dtors
// usually called from boot.S

#include "system/kernel_layout.hpp"

extern "C"
{


// IMPORTANT: initialization of libc components such as printf depends on this.
void call_ctors(void)
{
	for (auto ctor = &start_ctors; ctor != &end_ctors; ctor++)
	{
		(*ctor)();
	}
}

void call_dtors(void)
{
	for (auto dtor = &start_dtors; dtor != &end_dtors; dtor++)
	{
		(*dtor)();
	}
}

}
