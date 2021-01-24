#include "debug/backtrace.hpp"
#include "debug/kdebug.h"

#include "system/memlayout.h"

void kdebug::kdebug_print_backtrace()
{
	kdebug::kdebug_log("Call stack:\n");
	size_t counter = 0;
	for (auto frame = reinterpret_cast<uintptr_t**>(__builtin_frame_address(0));
	     (uintptr_t)frame && frame[0] != nullptr;
	     frame = reinterpret_cast<uintptr_t**>(frame[0]))
	{
		kdebug::kdebug_log(" 0x%p", frame[1]);
		if (++counter % 4 == 0)kdebug_log("\n");
	}
	kdebug::kdebug_log(".\n");
}

// read information from %rbp and retrive pcs
void kdebug::kdebug_get_caller_pcs(size_t buflen, uintptr_t* pcs)
{
	uintptr_t* ebp = nullptr;
	asm volatile("mov %%rbp, %0"
	: "=r"(ebp));

	size_t i = 0;
	for (; i < buflen; i++)
	{
		if (ebp == nullptr || ebp < (uintptr_t*)KERNEL_VIRTUALBASE || ebp == (uintptr_t*)VIRTUALADDR_LIMIT)
		{
			break;
		}
		pcs[i] = ebp[1];           // saved %eip
		ebp = (uintptr_t*)ebp[0]; // saved %ebp
	}

	for (; i < buflen; i++)
	{
		pcs[i] = 0;
	}
}