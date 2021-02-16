#include "arch/amd64/cpu/x86.h"
#include "arch/amd64/cpu/cls.hpp"
#include "arch/amd64/lock/arch_spinlock.hpp"

#include "drivers/console/console.h"

#include "arch/amd64/intrinsics.hpp"

#define  ARCH_SPINLOCK
#include "drivers/apic/traps.h"

//void lock::arch_spinlock_initialize_lock(arch_spinlock* lk, const char* name)
//{
//	lk->name = name;
//	lk->locked = 0u;
//	lk->cpu = nullptr;
//}
//
//void lock::arch_spinlock_acquire(arch_spinlock* lock)
//{
//	__sync_synchronize();
//
//	while (xchg(&lock->locked, 1u) != 0);
//}
//
//void lock::arch_spinlock_release(arch_spinlock* lock)
//{
//	__sync_synchronize();
//	xchg(&lock->locked, 0u);
//}

//TODO: temporary workaround
static inline uint8_t this_cpu_id()
{
	uint8_t* cpuid_ptr = ((uint8_t*)cls_get<uint8_t*>(CLS_CPU_STRUCT_PTR));
	if (!cpuid_ptr || ((uintptr_t)cpuid_ptr) < 0xFFFFFFFF80000000)
		return 0; //FIXME because some lock is called when there should not be locks
	return *cpuid_ptr;
}

void lock::arch_spinlock_lock(lock::arch_spinlock* lock)
{
	__sync_synchronize();

	uint64_t expected = 0, val = this_cpu_id() + 1;
	while (!__atomic_compare_exchange_n(&lock->value, &expected, val, false,
		__ATOMIC_ACQUIRE, __ATOMIC_RELAXED))
	{
		expected = 0;
		do
		{
			arch::cpu_yield();
		}
		while (__atomic_load_n(&lock->value, __ATOMIC_RELAXED) != 0);
	}
}

bool lock::arch_spinlock_try_lock(lock::arch_spinlock* lock)
{
	uint64_t val = this_cpu_id() + 1;

	uint64_t expected = 0;

	__atomic_compare_exchange_n(&lock->value, &expected, val, false, __ATOMIC_ACQUIRE,
		__ATOMIC_RELAXED);

	return expected != 0;
}

void lock::arch_spinlock_unlock(lock::arch_spinlock* lock)
{
	__atomic_store_n(&lock->value, 0UL, __ATOMIC_RELEASE);
}


