#pragma once
#include <cpuid.h>

#include "sys/types.h"

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

// // eax,ebx,ecx,edx
// static inline auto cpuid(uint32_t code, uint64_t regs[4]) -> decltype(regs[0])
// {
//   asm volatile("cpuid"
//                : "=a"(*regs), "=b"(*(regs + 1)),
//                  "=c"(*(regs + 2)), "=d"(*(regs + 3))
//                : "a"(code));
//   return regs[0];
// }

[[clang::optnone]] static inline cpuid_regs cpuid(cpuid_requests req)
{
  cpuid_regs ret = {0, 0, 0, 0};
  uint32_t code = (uint32_t)req;
  asm volatile("cpuid"
               : "=a"(ret.eax), "=b"(ret.ebx),
                 "=c"(ret.ecx), "=d"(ret.edx)
               : "a"(code));
  return ret;
}
