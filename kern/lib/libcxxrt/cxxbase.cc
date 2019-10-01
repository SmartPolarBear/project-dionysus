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

//Exceptions aren't supported in kernel, so we mark these operations noexcept
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wnew-returns-null"

void operator delete(void *ptr) noexcept
{
    //TODO: Implement later : Call kernel free
    ptr = nullptr;
}

void *operator new(size_t len)
{
    //TODO: Implement later : Call kernel allocate
    return nullptr;
}

void operator delete[](void *ptr) noexcept
{
    ::operator delete(ptr);
}

void *operator new[](size_t len)
{
    return ::operator new(len);
}

inline void *operator new(size_t, void *p) noexcept
{
    return p;
}

inline void *operator new[](size_t, void *p) noexcept
{
    return p;
}

inline void operator delete(void *, void *)noexcept
{
    //Do nothing
}

inline void operator delete[](void *, void *) noexcept
{
    //Do nothing
}

void *__gxx_personality_v0 = (void *)0xDEADBEAF;