#include "sys/types.h"

extern "C"
{

    int __cxa_atexit(void (*Destructor)(void *), void *Parameter, void *HomeDSO);
    void __cxa_finalize(void *);
    void __cxa_pure_virtual();
    void __stack_chk_guard_setup();
    void __attribute__((noreturn)) __stack_chk_fail();
    void _Unwind_Resume();
}

void *__dso_handle;
void *__stack_chk_guard(0);

namespace __cxxabiv1
{
__extension__ typedef int __guard __attribute__((mode(__DI__)));

extern "C"
{
    int __cxa_guard_acquire(__guard *Guard) { return !*(char *)(Guard); }
    void __cxa_guard_release(__guard *Guard) { *(char *)Guard = 1; }
    void __cxa_guard_abort(__guard *) {}
}
} // namespace __cxxabiv1

int __cxa_atexit(void (*)(void *), void *, void *)
{
    return 0;
}

void _Unwind_Resume()
{
}

void __cxa_finalize(void *)
{
}

void __cxa_pure_virtual()
{
}

void __stack_chk_guard_setup()
{
    unsigned char *Guard;
    Guard = (unsigned char *)&__stack_chk_guard;
    Guard[sizeof(__stack_chk_guard) - 1] = 255;
    Guard[sizeof(__stack_chk_guard) - 2] = '\n';
    Guard[0] = 0;
}

struct IntRegs;

void __attribute__((noreturn)) __stack_chk_fail()
{
    //TODO: Implement later: a error message
    // io.print("Buffer Overflow (SSP Signal)\n");
    for (;;)
        ;
}

void operator delete(void *ptr)
{
    //TODO: Implement later : Call kernel free
}

void *operator new(size_t len)
{
    //TODO: Implement later : Call kernel allocate
    return nullptr;
}

void operator delete[](void *ptr)
{
    ::operator delete(ptr);
}

void *operator new[](size_t len)
{
    return ::operator new(len);
}

void *__gxx_personality_v0 = (void *)0xDEADBEAF;