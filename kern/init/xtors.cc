// C++ ctors and dtors
// usually called from boot.S

extern "C"
{

using ctor_type = void (*)();
using dtor_type = void (*)();

extern ctor_type start_ctors;
extern ctor_type end_ctors;

extern dtor_type start_dtors;
extern dtor_type end_dtors;

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
