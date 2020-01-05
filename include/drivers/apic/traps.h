#if !defined(__INCLUDE_DIRVERS_APIC_TRAPS_H__)
#define __INCLUDE_DIRVERS_APIC_TRAPS_H__

#include "arch/amd64/regs.h"

#include "drivers/debug/kdebug.h"

namespace trap
{
constexpr size_t TRAP_NUMBERMAX = 256;
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

    // These are arbitrarily chosen, but with care not to overlap
    // processor defined exceptions or interrupt vectors.
    TRAP_SYSCALL = 64,  // system call
    TRAP_DEFAULT = 500, // catchall

    TRAP_IRQ0 = 32, // IRQ 0 corresponds to int T_IRQ
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

using trap_handle_func = hresult (*)(trap_info);

struct trap_handle
{
    trap_handle_func handle;
};

void initialize_trap_vectors(void);

// returns the old handle
trap_handle trap_handle_regsiter(size_t trapnumber, trap_handle handle);

} // namespace trap

#endif // __INCLUDE_DIRVERS_APIC_TRAPS_H__
