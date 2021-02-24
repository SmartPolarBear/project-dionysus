#pragma once

#include "object/kernel_object.hpp"

#include "system/types.h"

#include "kbl/lock/spinlock.h"
#include "ktl/shared_ptr.hpp"
#include "ktl/unique_ptr.hpp"

#include "kbl/checker/canary.hpp"

namespace object
{

enum class [[clang::enum_extensibility(closed)]] object_type
{
	PROCESS = 1,
	JOB,
	THREAD,
};

template<typename T>
struct dispatcher_tag;

template<typename T>
struct canary_tag;

#ifdef DECLARE_TAG
#error "DECLARE_TAG should not have been defined"
#endif
#define DECLARE_TAG(TYPE, ID, MAGIC) \
    class TYPE;                       \
    template<>                          \
    struct dispatcher_tag<TYPE>         \
    {                                   \
    static constexpr object_type type=ID;  \
    };                                  \
    template<>                          \
    struct canary_tag<TYPE>             \
    {static constexpr auto magic=kbl::magic(MAGIC);};  \


DECLARE_TAG(job_dispatcher, object_type::JOB, "JOB_")
DECLARE_TAG(process_dispatcher, object_type::PROCESS, "PROC")
DECLARE_TAG(thread, object_type::THREAD, "THRD")

#undef DECLARE_TAG

using right_type = uint64_t;

class dispatcher :
	public kernel_object<dispatcher>
{
 public:
	constexpr dispatcher() = default;

	virtual ~dispatcher() = default;

	virtual object_type get_type() const= 0;
};

template<typename T, right_type rights>
class solo_dispatcher
	: public dispatcher
{
 protected:
	[[nodiscard]]lock::spinlock* get_lock()
	{
		return &lock;
	}

	mutable lock::spinlock lock{ "dispatcher" };
};

template<typename T>
static inline ktl::shared_ptr<T> downcast_dispatcher(ktl::shared_ptr<dispatcher>* d)
{
	if ((*d)->get_type() == dispatcher_tag<T>::type)[[likely]]
	{
		return ktl::shared_ptr<T>{ reinterpret_cast<T*>(d->get()) };
	}
	else [[unlikely]]
	{
		return nullptr;
	}
}

template<>
static inline ktl::shared_ptr<dispatcher> downcast_dispatcher(ktl::shared_ptr<dispatcher>* d)
{
	return std::move(*d);
}

template<typename T>
static inline T* downcast_dispatcher(dispatcher* d)
{
	if (d->get_type() == dispatcher_tag<T>::type)[[likely]]
	{
		return ktl::shared_ptr<T>{ reinterpret_cast<T*>(d) };
	}
	else [[unlikely]]
	{
		return nullptr;
	}
}

template<>
static inline dispatcher* downcast_dispatcher(dispatcher* d)
{
	return d;
}

template<typename T>
static inline const T* downcast_dispatcher(const dispatcher* d)
{
	if (d->get_type() == dispatcher_tag<T>::type)[[likely]]
	{
		return ktl::shared_ptr<T>{ reinterpret_cast<T*>(d) };
	}
	else [[unlikely]]
	{
		return nullptr;
	}
}

template<>
static inline const dispatcher* downcast_dispatcher(const dispatcher* d)
{
	return d;
}

}