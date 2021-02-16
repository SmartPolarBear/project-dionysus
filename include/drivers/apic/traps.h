#pragma once

#include "arch/amd64/cpu/regs.h"
#include "arch/amd64/cpu/cpu.h"

#if !defined(ARCH_SPINLOCK)
#include "debug/kdebug.h"
#endif

// defined in trapentry_asm.S
extern "C" void trap_entry();
// extern "C" void trap_ret();

namespace trap
{
	constexpr size_t TRAP_NUMBERMAX = 256;
	constexpr uint16_t TRAP_MSI_BASE = 0x80;

// Processor-defined:
	enum processor_defined_traps
	{
		TRAP_DIVIDE = 0,   // divide error
		TRAP_DEBUG = 1,    // debug exception
		TRAP_NMI = 2,      // non-maskable interrupt
		TRAP_BRKPT = 3,    // breakpoint
		TRAP_OFLOW = 4,    // overflow
		TRAP_BOUND = 5,    // bounds check
		TRAP_ILLOP = 6,    // illegal opcode
		TRAP_DEVICE = 7,   // device not available
		TRAP_DBLFLT = 8,   // double fault
		TRAP_COPROC = 9,   // reserved (not used since 486)
		TRAP_TSS = 10,     // invalid task switch segment
		TRAP_SEGNP = 11,   // segment not present
		TRAP_STACK = 12,   // stack exception
		TRAP_GPFLT = 13,   // general protection fault
		TRAP_PGFLT = 14,   // page fault
		TRAP_RES = 15,     // reserved
		TRAP_FPERR = 16,   // floating point error
		TRAP_ALIGN = 17,   // aligment check
		TRAP_MCHK = 18,    // machine check
		TRAP_SIMDERR = 19, // SIMD floating point error
		TRAP_VIRTUALIZATION = 20,
		TRAP_SECURITY = 30,

		// These are arbitrarily chosen, but with care not to overlap
		// processor defined exceptions or interrupt vectors.
		TRAP_DEFAULT = 500, // catchall

		TRAP_IRQ0 = 32, // IRQ 0 corresponds to int T_IRQ
	};



	struct trap_frame
	{
		uint64_t rax;
		uint64_t rbx;
		uint64_t rcx;
		uint64_t rdx;
		uint64_t rbp;
		uint64_t rsi;
		uint64_t rdi;
		uint64_t r8;
		uint64_t r9;
		uint64_t r10;
		uint64_t r11;
		uint64_t r12;
		uint64_t r13;
		uint64_t r14;
		uint64_t r15;

		uint64_t trap_num;
		uint64_t err;

		uint64_t rip;
		uint64_t cs;
		uint64_t rflags;
		uint64_t rsp;
		uint64_t ss;     // ds in x86
	};

	enum irqs
	{
		IRQ_TIMER = 0,
		IRQ_KBD = 1,
		IRQ_COM1 = 4,
		IRQ_IDE = 14,
		IRQ_ERROR = 19,
		IRQ_SPURIOUS = 31,
	};

	static inline constexpr size_t irq_to_trap_number(irqs irq)
	{
		return static_cast<size_t>(irq) + static_cast<size_t>(TRAP_IRQ0);
	}

	using trap_handle_func = error_code (*)(trap_frame);

	struct trap_handle
	{
		trap_handle_func handle;
		bool enable;
	};

	PANIC void init_trap(void);

// returns the old handle
	PANIC trap_handle trap_handle_register(size_t trapnumber, trap_handle handle);

// enable the trap handle, returns the old state
	PANIC bool trap_handle_enable(size_t trapnumber, bool enable);

	PANIC void pushcli(void);
	PANIC void popcli(void);

	void print_trap_frame(IN const trap_frame* frm);

} // namespace trap

static inline bool
__intr_save(void)
{
	if (read_eflags() & EFLAG_IF)
	{
		cli();
		return 1;
	}
	return 0;
}

static inline void
__intr_restore(bool flag)
{
	if (flag)
	{
		sti();
	}
}

#define save_intr(x) \
    do                      \
    {                       \
        x = __intr_save();  \
    } while (0)

#define restore_intr(x) \
    do                     \
    {                      \
        __intr_restore(x); \
    } while (0)
