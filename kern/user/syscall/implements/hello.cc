#include "syscall.h"

#include "syscall/syscall_args.hpp"

#include "system/mmu.h"
#include "system/syscall.h"

#include "debug/kdebug.h"
#include "drivers/apic/traps.h"

#include "arch/amd64/cpu/cpuid.h"
#include "arch/amd64/cpu/msr.h"
#include "arch/amd64/cpu/regs.h"

#include "builtin_text_io.hpp"

#include "task/process/process.hpp"
using namespace syscall;

error_code sys_hello(const syscall_regs* regs)
{

	write_format("[pid %d]hello ! %lld %lld %lld %lld\n",
		cur_proc->get_koid(),
		args_get<0>(regs),
		args_get<1>(regs),
		args_get<2>(regs),
		args_get<3>(regs));

	return ERROR_SUCCESS;
}