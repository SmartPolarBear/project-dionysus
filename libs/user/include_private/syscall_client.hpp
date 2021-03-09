//
// Created by bear on 5/31/20.
//

#pragma once

#include "system/syscall.h"
#include "system/error.hpp"

#include <tuple>
#include <concepts>

#ifdef _SYSCALL_ASM_CLOBBERS
#error "_SYSCALL_ASM_CLOBBERS should not have been defined"
#endif

#define _SYSCALL_ASM_CLOBBERS "rsi","rdi","rdx","r10","r9","r8"

namespace _internals
{
/// \brief convert the parameter pack to a array of given type T and size Size at compile time
/// \tparam T type, all elements of parameter pack should be able to be cast to T
/// \tparam Size size of the parameter_pack
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

template<typename T>
concept syscall_argument=
requires(T t)
{
	{ ((uint64_t)t) }->std::convertible_to<uint64_t>;
};

/// \brief syscall without out parameters
/// \param syscall_number the syscall number, satisfying 0<=syscall_number<SYSCALL_COUNT or ERROR_INVALID is returned
/// \param args arguments for syscall. 0<=[the count of parameter]<7 and all parameter can be cast to uint64_t
/// \return the error_code of syscall
__attribute__((always_inline))  static inline error_code make_syscall(uint64_t syscall_number,
	syscall_argument auto ...args)
{
	constexpr size_t ARG_COUNT = sizeof...(args);

	if (syscall_number >= syscall::SYSCALL_COUNT_MAX ||
		ARG_COUNT > syscall::SYSCALL_PARAMETER_MAX)
	{
		return -ERROR_INVALID;
	}

	_internals::parameter_pack<uint64_t, ARG_COUNT> pack{ args... };

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