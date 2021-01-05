#pragma once
namespace kdebug
{

#if defined(__clang__)

#define NONNULL _Nonnull
#define NULLABLE _Nullable
#define NULL_UNSPECIFIED _Null_unspecified
#define NULLABLE_RESULT _Nullable_result
#define NONNULL_RESULT __attribute__((returns_nonnull))

#else

#define NONNULL
#define NULLABLE
#define NULL_UNSPECIFIED
#define NULLABLE_RESULT
#endif
}
