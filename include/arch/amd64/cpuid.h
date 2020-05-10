#pragma once
#include <cpuid.h>

#include "system/types.h"

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
  cpuid_regs ret = {0, 0, 0, 0};
  uint32_t code = (uint32_t)req;
  asm volatile("cpuid"
               : "=a"(ret.eax), "=b"(ret.ebx),
                 "=c"(ret.ecx), "=d"(ret.edx)
               : "a"(code));
  return ret;
}
