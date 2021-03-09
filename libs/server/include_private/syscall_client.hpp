//
// Created by bear on 5/31/20.
//

#pragma once

#include "system/syscall.h"
#include "system/error.hpp"

#include <tuple>

#ifdef _SYSCALL_ASM_CLOBBERS
#error "_SYSCALL_ASM_CLOBBERS should not be defined"
#endif

#define _SYSCALL_ASM_CLOBBERS "rsi","rdi","rdx","r10","r9","r8"

namespace _internals
{
template<typename T, std::size_t Size>
struct parameter_pack
{
	T data[Size]{};

	template<typename ...Args>
	constexpr explicit parameter_pack(const Args& ... args) : data{ ((T)(args))... }
	{
	}
};
}

// syscall without out parameters
// precondition:
//  1) 0<=syscall_number<SYSCALL_COUNT
//  2) 0<=para_count<7 and all parameter is uint64_t or those with the same size.

template<typename ... TArgs>
__attribute__((always_inline))  static inline error_code make_syscall(uint64_t syscall_number, TArgs ...args)
{
	constexpr size_t ARG_COUNT = sizeof...(TArgs);

	if (syscall_number >= syscall::SYSCALL_COUNT_MAX ||
		ARG_COUNT > syscall::SYSCALL_PARAMETER_MAX)
	{
		return -ERROR_INVALID;
	}

	_internals::parameter_pack<uint64_t, 6> pack{ args... };

	error_code ret = 0;

	switch (ARG_COUNT)
	{
	default:
		return -ERROR_INVALID;
		break;

	case 6:
		asm volatile("mov %[arg5], %%r9\n\t"
		:
		:  [arg5] "r"(pack.data[5])
		: _SYSCALL_ASM_CLOBBERS, "cc", "memory");
		[[fallthrough]];
	case 5:
		asm volatile("mov %[arg4], %%r8\n\t"
		:
		: [arg4] "r"(pack.data[4])
		: _SYSCALL_ASM_CLOBBERS, "cc", "memory");
		[[fallthrough]];
	case 4:
		asm volatile("mov %[arg3], %%r10\n\t"
		:
		: [arg3] "r"(pack.data[3])
		:  _SYSCALL_ASM_CLOBBERS, "cc", "memory");
		[[fallthrough]];

	case 3:
		asm volatile("mov %[arg2], %%rdx\n\t"
		:
		: [arg2] "r"(pack.data[2])
		:  _SYSCALL_ASM_CLOBBERS, "cc", "memory");
		[[fallthrough]];

	case 2:
		asm volatile("mov %[arg1], %%rsi\n\t"
		:
		: [arg1]"r"(pack.data[1])
		: _SYSCALL_ASM_CLOBBERS, "cc", "memory");
		[[fallthrough]];

	case 1:
		asm volatile("mov %[arg0], %%rdi\n\t"
		:
		: [arg0] "r"(pack.data[0])
		: _SYSCALL_ASM_CLOBBERS, "cc", "memory");
		[[fallthrough]];

	case 0:
		break;

	}

	// rcx and r11 are used by syscall instruction and therefore should be protected
	asm volatile ( "syscall"
	: "=a" (ret)
	: "a" (syscall_number)
	: _SYSCALL_ASM_CLOBBERS, "rcx", "r11", "rbx", "cc", "memory"  );

	return ret;
}

#undef _SYSCALL_ASM_CLOBBERS