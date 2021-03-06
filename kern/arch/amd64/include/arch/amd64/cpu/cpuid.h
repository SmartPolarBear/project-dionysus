#pragma once
#include <cpuid.h>

#include "system/types.h"

namespace features
{
/* Features in %ecx for level 1 */
enum ecx_bits
{
	CPUID_ECX_BIT_SSE3 = 0x00000001,
	CPUID_ECX_BIT_PCLMULQDQ = 0x00000002,
	CPUID_ECX_BIT_DTES64 = 0x00000004,
	CPUID_ECX_BIT_MONITOR = 0x00000008,
	CPUID_ECX_BIT_DSCPL = 0x00000010,
	CPUID_ECX_BIT_VMX = 0x00000020,
	CPUID_ECX_BIT_SMX = 0x00000040,
	CPUID_ECX_BIT_EIST = 0x00000080,
	CPUID_ECX_BIT_TM2 = 0x00000100,
	CPUID_ECX_BIT_SSSE3 = 0x00000200,
	CPUID_ECX_BIT_CNXTID = 0x00000400,
	CPUID_ECX_BIT_FMA = 0x00001000,
	CPUID_ECX_BIT_CMPXCHG16B = 0x00002000,
	CPUID_ECX_BIT_xTPR = 0x00004000,
	CPUID_ECX_BIT_PDCM = 0x00008000,
	CPUID_ECX_BIT_PCID = 0x00020000,
	CPUID_ECX_BIT_DCA = 0x00040000,
	CPUID_ECX_BIT_SSE41 = 0x00080000,
	CPUID_ECX_BIT_SSE42 = 0x00100000,
	CPUID_ECX_BIT_x2APIC = 0x00200000,
	CPUID_ECX_BIT_MOVBE = 0x00400000,
	CPUID_ECX_BIT_POPCNT = 0x00800000,
	CPUID_ECX_BIT_TSCDeadline = 0x01000000,
	CPUID_ECX_BIT_AESNI = 0x02000000,
	CPUID_ECX_BIT_XSAVE = 0x04000000,
	CPUID_ECX_BIT_OSXSAVE = 0x08000000,
	CPUID_ECX_BIT_AVX = 0x10000000,
	CPUID_ECX_BIT_RDRAND = 0x40000000,
};

/* Features in %edx for level 1 */
enum edx_bits
{
	CPUID_EDX_BIT_FPU = 0x00000001,
	CPUID_EDX_BIT_VME = 0x00000002,
	CPUID_EDX_BIT_DE = 0x00000004,
	CPUID_EDX_BIT_PSE = 0x00000008,
	CPUID_EDX_BIT_TSC = 0x00000010,
	CPUID_EDX_BIT_MSR = 0x00000020,
	CPUID_EDX_BIT_PAE = 0x00000040,
	CPUID_EDX_BIT_MCE = 0x00000080,
	CPUID_EDX_BIT_CX8 = 0x00000100,
	CPUID_EDX_BIT_APIC = 0x00000200,
	CPUID_EDX_BIT_SEP = 0x00000800,
	CPUID_EDX_BIT_MTRR = 0x00001000,
	CPUID_EDX_BIT_PGE = 0x00002000,
	CPUID_EDX_BIT_MCA = 0x00004000,
	CPUID_EDX_BIT_CMOV = 0x00008000,
	CPUID_EDX_BIT_PAT = 0x00010000,
	CPUID_EDX_BIT_PSE36 = 0x00020000,
	CPUID_EDX_BIT_PSN = 0x00040000,
	CPUID_EDX_BIT_CLFSH = 0x00080000,
	CPUID_EDX_BIT_DS = 0x00200000,
	CPUID_EDX_BIT_ACPI = 0x00400000,
	CPUID_EDX_BIT_MMX = 0x00800000,
	CPUID_EDX_BIT_FXSR = 0x01000000,
	CPUID_EDX_BIT_SSE = 0x02000000,
	CPUID_EDX_BIT_SSE2 = 0x04000000,
	CPUID_EDX_BIT_SS = 0x08000000,
	CPUID_EDX_BIT_HTT = 0x10000000,
	CPUID_EDX_BIT_TM = 0x20000000,
	CPUID_EDX_BIT_PBE = 0x80000000,
};


/* Features in %ebx for level 7 sub-leaf 0 */
//TODO
//#define bit_FSGSBASE    0x00000001
//#define bit_SMEP        0x00000080
//#define bit_ENH_MOVSB   0x00000200
}

enum cpuid_requests
{
	CPUID_GETVENDORSTRING,
	CPUID_GETFEATURES,
	CPUID_GETTLB,
	CPUID_GETSERIAL,

	CPUID_INTELEXTENDED = 0x80000000,
	CPUID_INTELFEATURES,
	CPUID_INTELBRANDSTRING,
	CPUID_INTELBRANDSTRINGMORE,
	CPUID_INTELBRANDSTRINGEND,
};

struct cpuid_regs
{
	uint64_t eax, ebx, ecx, edx;
};

[[clang::optnone]] static inline cpuid_regs cpuid(cpuid_requests req)
{
	cpuid_regs ret = { 0, 0, 0, 0 };
	uint32_t code = (uint32_t)req;
	asm volatile("cpuid"
	: "=a"(ret.eax), "=b"(ret.ebx),
	"=c"(ret.ecx), "=d"(ret.edx)
	: "a"(code));
	return ret;
}
