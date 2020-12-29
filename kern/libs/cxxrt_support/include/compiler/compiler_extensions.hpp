#pragma once

#if !defined(__clang__)
#error "Only clang can be used!"
#endif

#if !defined(__ASSEMBLER__)

// C++ header guards.
#ifdef __cplusplus
#define __BEGIN_CDECLS extern "C" {
#define __END_CDECLS }
#else
#define __BEGIN_CDECLS
#define __END_CDECLS
#endif

//
// Function and data attributes.
//

// Function inlining directives.
#define __NO_INLINE __attribute__((__noinline__))
#define __ALWAYS_INLINE __attribute__((__always_inline__))

// Avoid issuing a warning if the given variable/function is unused.
#define __UNUSED __attribute__((__unused__))

// Pack the given structure or class, omitting padding between fields.
#define __PACKED __attribute__((packed))

// Place the variable in thread-local storage.
#define __THREAD __thread

// Align the variable or type to at least `x` bytes. `x` must be a power of two.
#define __ALIGNED(x) __attribute__((aligned(x)))

// Declare the given function will never return. (such as `exit`, `abort`, etc).
#define __NO_RETURN __attribute__((__noreturn__))

// Warn if the result of this function is ignored by the caller.
#define __WARN_UNUSED_RESULT __attribute__((__warn_unused_result__))


// The given function is malloc-like, returning a pointer to new, unused
// memory.
//
// The compiler can assume that the returned pointer does not alias any other
// pointer, which may help the compiler optimize the program.
#define __MALLOC __attribute__((__malloc__))

// Indicate that the given function takes a printf/scanf-style format string.
//
// "__fmt" is the argument number of the format string, indexed from 1.
// "__varargs" is the argument number of the variable args "..." argument
// counting from 1.
//
// If applied to a class method, the implicit "this" parameter counts as the
// first argument.
#define __PRINTFLIKE(__fmt, __varargs) __attribute__((__format__(__printf__, __fmt, __varargs)))
#define __SCANFLIKE(__fmt, __varargs) __attribute__((__format__(__scanf__, __fmt, __varargs)))

// Indicate that the `n`th argument to a function is non-null.
//
// The compiler will emit warnings if it can prove an argument is null, and
// may optimise assuming that the values are non-null.
#define __NONNULL(n) __attribute__((__nonnull__ n))

// The given function is a "leaf", and won't call further functions.
//
// Leaf functions must only return directly, and not call back into the
// current compilation unit (either via direct calls, or function pointers).
//
// May help the compiler optimize calls to the function in some cases.
#if !defined(__clang__)
#define __LEAF_FN __attribute__((__leaf__))
#else
#define __LEAF_FN
#endif


// Optimize the given function using custom flags.
//
// For example,
//
//  __OPTIMIZE("O3") int myfunction() { ... }
//
// will cause GCC to optimize the function at the O3 level, independent
// of what the compiler optimization flags are.
#if !defined(__clang__)
#define __OPTIMIZE(x) __attribute__((__optimize__(x)))
#else
#define __OPTIMIZE(x)
#endif

// Indicate the given function should not use LLVM's stack hardening features,
// but instead put all local variables on the standard stack.
//
// c.f. https://clang.llvm.org/docs/SafeStack.html
#if defined(__clang__)
#define __NO_SAFESTACK __attribute__((__no_sanitize__("safe-stack", "shadow-call-stack")))
#else
#define __NO_SAFESTACK
#endif


// Indicate that the given function should be treated by the Clang static
// analyzer as if it doesn't return.
//
// A workaround to help static analyzer identify assertion failures
#if defined(__clang__)
#define __ANALYZER_CREATE_SINK __attribute__((analyzer_noreturn))
#else
#define __ANALYZER_CREATE_SINK
#endif


// Declare that this function declaration should be emitted as an alias for
// another function.
#define __ALIAS(x) __attribute__((__alias__(x)))

// Place the given global into a particular linker section.
#define __SECTION(x) __attribute__((__section__(x)))

// The given function or global should be given a weak symbol, or a weak
// alias to another symbol.
#define __WEAK __attribute__((__weak__))
#define __WEAK_ALIAS(x) __attribute__((__weak__, __alias__(x)))

// The given static variable should still be emitted by the compiler, even if it
// appears unused to the compiler.
//
// Rarely needed. Not to be confused with "__UNUSED", which avoids the
// compiler warning if a variable appears unused.
#define __ALWAYS_EMIT __attribute__((__used__))

// Declare this object's ELF symbol visibility.
//#define __EXPORT __attribute__((__visibility__("default")))
//#define __LOCAL __attribute__((__visibility__("hidden")))

//
// Builtin functions.
//

// Provide a hint to the compiler that the given expression is likely/unlikely
// to be true.
//
//   if (unlikely(status != ZX_OK)) {
//     error(...);
//   }
//
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

// Return the program counter of the calling function.
#define __GET_CALLER(x) __builtin_return_address(0)

// Return the address of the current stack frame.
#define __GET_FRAME(x) __builtin_frame_address(0)

// Return true if the given expression is a known compile-time constant.
#define __ISCONSTANT(x) __builtin_constant_p(x)

// Assume this branch of code cannot be reached.
#define __UNREACHABLE __builtin_unreachable()

// Get the offset of `field` from the beginning of the struct or class `type`.
#if defined (__offsetof)
#undef __offsetof
#define __offsetof(type, field) __builtin_offsetof(type, field)
#endif

// Perform an arithmetic operation, returning "true" if the operation overflowed.
//
// Equivalent to: { *result = a + b; return _overflow_occurred; }
#define add_overflow(a, b, result) __builtin_add_overflow(a, b, result)
#define sub_overflow(a, b, result) __builtin_sub_overflow(a, b, result)
#define mul_overflow(a, b, result) __builtin_mul_overflow(a, b, result)


// Experimental lifetime analysis annotations.
#ifndef __OWNER
#ifdef __clang__
#define __OWNER(x) [[gsl::Owner(x)]]
#define __POINTER(x) [[gsl::Pointer(x)]]
#else
#define __OWNER(x)
#define __POINTER(x)
#endif
#endif

#endif // !defined(__ASSEMBLER__)
