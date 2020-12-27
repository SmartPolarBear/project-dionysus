#include "arch/amd64/cpu/cpuid.h"
#include "arch/amd64/cpu/regs.h"
#include "arch/amd64/cpu/cpu.h"

#include "drivers/simd/simd.hpp"

#include "system/error.hpp"

#include "../../libs/basic_io/include/builtin_text_io.hpp"

error_code simd::enable_simd()
{
	// check CPUID for availability
	auto[eax, ebx, ecx, edx]=cpuid(CPUID_GETFEATURES);

	// SEE status is bit 25
	if (edx & (1 << 25))
	{
		auto cr0 = rcr0();
		cr0 &= (~(1 << 2));
		cr0 |= (1 << 1);

		auto cr4 = rcr4();
		cr4 |= (1 << 9);
		cr4 |= (1 << 10);

		lcr0(cr0);
		lcr4(cr4);
	}

	// AVX status is on ECX bit 28
	if (ecx & (1 << 28))
	{
		enable_avx();
	}

	return ERROR_SUCCESS;
}