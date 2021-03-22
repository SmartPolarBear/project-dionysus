#pragma once

#include "system/types.h"

#include "object/ref_counted.hpp"

#include "ktl/atomic.hpp"

#include "kbl/singleton/singleton.hpp"

#include "object/public/kernel_object.hpp"

namespace object
{


class koid_allocator final
{
 public:
	using koid_counter_type = ktl::atomic<koid_type>;
	static_assert(koid_counter_type::is_always_lock_free);

	[[nodiscard]] constexpr koid_type fetch()
	{
		return counter_.fetch_add(1);
	}

	static koid_allocator& instance();

 private:
	constexpr explicit koid_allocator(koid_type start)
	{
		counter_.store(start);
	}

	koid_counter_type counter_{ 0 };
};

class basic_object
{
 public:
	static uint64_t get_object_count()
	{
		return obj_counter_.load();
	}

	basic_object()
	{
		++obj_counter_;
	}

	virtual ~basic_object()
	{
		--obj_counter_;
	}

 private:
	static ktl::atomic<size_t> obj_counter_;
};

template<typename T>
class kernel_object :
	public object::basic_object,
	public object::ref_counted
{
 public:

	constexpr kernel_object()
		: object::basic_object(),
		  koid_{ koid_allocator::instance().fetch() }
	{
	}

	virtual ~kernel_object() = default;

	[[nodiscard]] koid_type get_koid() const
	{
		return koid_;
	}

 private:

	koid_type koid_{ 0 };

};

}