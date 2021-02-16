#pragma once

#include "arch/amd64/cpu/x86.h"

typedef uint64_t x86_flags_t;

static inline uint64_t x86_save_flags(void)
{
	uint64_t state;

	__asm__ volatile(
	"pushfq;"
	"popq %0"
	: "=rm"(state)::"memory");

	return state;
}

static inline void x86_restore_flags(uint64_t flags)
{
	__asm__ volatile(
	"pushq %0;"
	"popfq"::"g"(flags)
	: "memory", "cc");
}

extern "C" void cli();
extern "C" void sti();

enum [[clang::flag_enum, clang::enum_extensibility(open)]] rflags_value
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
	EFLAG_ID = 0x00200000,        // ID flags_
};

typedef uint64_t interrupt_saved_state_type;

[[nodiscard]] static inline interrupt_saved_state_type arch_interrupt_save()
{
	interrupt_saved_state_type state = x86_save_flags();
	if ((state & EFLAG_IF) != 0)
	{
		cli();
	}

	__atomic_signal_fence(__ATOMIC_SEQ_CST);

	return state;
}

static inline void arch_interrupt_restore(interrupt_saved_state_type state)
{
	__atomic_signal_fence(__ATOMIC_SEQ_CST);

	if ((state & EFLAG_IF) != 0)
	{
		sti();
	}
}
