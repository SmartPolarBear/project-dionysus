#include "debug/backtrace.hpp"
#include "debug/kdebug.h"

#include "system/memlayout.h"

static __always_inline uint64_t
read_rbp()
{
	uint64_t rbp;
	asm volatile ("movq %%rbp, %0" : "=r" (rbp));
	return rbp;
}

static __noinline uint64_t
read_rip()
{
	uint64_t rip;
	asm volatile("movq 8(%%rbp), %0" : "=r" (rip));
	return rip;
}

void kdebug::kdebug_print_backtrace()
{

	uint64_t rbp = read_rbp(), rip = read_rip();

	size_t counter = 0;
	for (size_t i = 0; rbp != 0 && i <= 20; i++)
	{
		kdebug_log(" 0x%x", rip);
		if (++counter % 4 == 0)kdebug_log("\n");
		rip = ((uint64_t*)rbp)[1];
		rbp = ((uint64_t*)rbp)[0];

	}

//	size_t counter = 0;
//	for (auto frame = reinterpret_cast<uintptr_t**>(__builtin_frame_address(0));
//	     frame != nullptr && frame[0] != nullptr;
//	     frame = reinterpret_cast<uintptr_t**>(frame[0]))
//	{
//		kdebug::kdebug_log(" 0x%p", frame[1]);
//		if (++counter % 4 == 0)kdebug_log("\n");
//	}
//	kdebug::kdebug_log(".\n");
}

// read information from %rbp and retrive pcs
size_t kdebug::kdebug_get_backtrace(uintptr_t* pcs)
{
	pcs[0]=0;
	return 0;
	uint64_t rbp = read_rbp(), rip = read_rip();

	size_t counter = 0;
	for (size_t i = 0; rbp != 0 && i <= 20; i++)
	{
		if (pcs)
		{
			pcs[counter++] = rip;
		}

		rip = ((uint64_t*)rbp)[1];
		rbp = ((uint64_t*)rbp)[0];

		if (counter > 20)return counter;

	}


	pcs[counter] = 0;

	return counter;

//	size_t counter = 0;
//	for (auto frame = reinterpret_cast<uintptr_t**>(__builtin_frame_address(0));
//	     (uintptr_t)frame && frame[0] != nullptr;
//	     frame = reinterpret_cast<uintptr_t**>(frame[0]))
//	{
//		uintptr_t val = (uintptr_t)frame[1];
//
//		if (pcs)
//		{
//			pcs[counter++] = val;
//		}
//
//		if (counter > 20)return counter;
//	}
//
//	pcs[counter] = 0;
//
//	return counter;
}