#pragma once

#include "arch/amd64/regs.h"

#include "debug/kdebug.h"

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

	enum rflags_value
	{
		// Eflags register
		EFLAG_CF = 0x00000001,        // Carry Flag
		EFLAG_PF = 0x00000004,        // Parity Flag
		EFLAG_AF = 0x00000010,        // Auxiliary carry Flag
		EFLAG_ZF = 0x00000040,        // Zero Flag
		EFLAG_SF = 0x00000080,        // Sign Flag
		EFLAG_TF = 0x00000100,        // Trap Flag
		EFLAG_IF = 0x00000200,        // Interrupt Enable
		EFLAG_DF = 0x00000400,        // Direction Flag
		EFLAG_OF = 0x00000800,        // Overflow Flag
		EFLAG_IOPL_MASK = 0x00003000, // I/O Privilege Level bitmask
		EFLAG_IOPL_0 = 0x00000000,    //   IOPL == 0
		EFLAG_IOPL_1 = 0x00001000,    //   IOPL == 1
		EFLAG_IOPL_2 = 0x00002000,    //   IOPL == 2
		EFLAG_IOPL_3 = 0x00003000,    //   IOPL == 3
		EFLAG_NT = 0x00004000,        // Nested Task
		EFLAG_RF = 0x00010000,        // Resume Flag
		EFLAG_VM = 0x00020000,        // Virtual 8086 mode
		EFLAG_AC = 0x00040000,        // Alignment Check
		EFLAG_VIF = 0x00080000,       // Virtual Interrupt Flag
		EFLAG_VIP = 0x00100000,       // Virtual Interrupt Pending
		EFLAG_ID = 0x00200000,        // ID flag
	};

	struct trap_frame
	{
		uint64_t rax; // eax in x86
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

		uint64_t rip; // eip in x86
		uint64_t cs;
		uint64_t rflags; // eflags in x86
		uint64_t rsp;    // esp in x86
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
	if (read_eflags() & trap::EFLAG_IF)
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
