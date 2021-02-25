#pragma once

#include "syscall/syscall.hpp"

#include "debug/kdebug.h"

#include "ktl/concepts.hpp"

namespace syscall
{

static inline constexpr int ARG_SYSCALL_NUM = -1;

uint64_t args_get(const syscall_regs* regs, int idx);

template<int ArgIdx>
static inline uint64_t args_get(const syscall_regs* regs);

namespace _internals
{

template<int I>
concept RegisterPassedArgument=I >= 0 && I <= 5;

template<int idx>
requires RegisterPassedArgument<idx>
static inline uint64_t do_get(const syscall_regs* regs)
{
	if constexpr (idx == 0)
	{
		return regs->rdi;
	}
	else if constexpr (idx == 1)
	{
		return regs->rsi;

	}
	else if constexpr (idx == 2)
	{
		return regs->rdx;

	}
	else if constexpr (idx == 3)
	{
		return regs->r10;

	}
	else if constexpr (idx == 4)
	{
		return regs->r8;

	}
	else if constexpr (idx == 5)
	{
		return regs->r9;
	}
	else
	{
		KDEBUG_GERNERALPANIC_CODE(-ERROR_SHOULD_NOT_REACH_HERE);
	}
}

template<int idx>
static inline uint64_t do_get(const syscall_regs* regs)
{
	syscall::args_get(regs, idx);
}

} // _internals

template<>
[[maybe_unused]] uint64_t args_get<syscall::ARG_SYSCALL_NUM>(const syscall_regs* regs)
{
	return regs->rax;
}

template<int Idx>
uint64_t args_get(const syscall_regs* regs)
{
	return _internals::do_get<Idx>(regs);
}

template<typename T, int ArgIdx>
requires std::convertible_to<uint64_t, T>
static inline T args_get(const syscall_regs* regs)
{
	return static_cast<T>(args_get<ArgIdx>(regs));
}

template<typename T>
requires std::convertible_to<uint64_t, T>
static inline T args_get(const syscall_regs* regs, int idx)
{
	return static_cast<T>(args_get(regs, idx));
}

template<typename T, int ArgIdx>
requires std::is_pointer_v<T>
static inline T args_get(const syscall_regs* regs)
{
	return reinterpret_cast<T>(args_get<ArgIdx>(regs));
}

template<typename T>
requires std::is_pointer_v<T>
static inline T args_get(const syscall_regs* regs, int idx)
{
	return reinterpret_cast<T>(args_get(regs, idx));
}

}