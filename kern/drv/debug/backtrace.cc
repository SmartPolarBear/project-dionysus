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
		uintptr_t val = (uintptr_t)frame[1];
		kdebug::kdebug_log(" 0x%p", frame[1]);
		if (++counter % 4 == 0)kdebug_log("\n");
	}
	kdebug::kdebug_log(".\n");
}

// read information from %rbp and retrive pcs
size_t kdebug::kdebug_get_backtrace(uintptr_t* pcs)
{
	size_t counter = 0;
	for (auto frame = reinterpret_cast<uintptr_t**>(__builtin_frame_address(0));
	     (uintptr_t)frame && frame[0] != nullptr;
	     frame = reinterpret_cast<uintptr_t**>(frame[0]))
	{
		uintptr_t val = (uintptr_t)frame[1];

		if (pcs)
		{
			pcs[counter++] = val;
		}

		if (counter > 20)return counter;
	}

	pcs[counter] = 0;

	return counter;
}