#pragma once

#include "system/types.h"

#include "compiler/compiler_extensions.hpp"

using timestamp_type = uint64_t;

using time_type = int64_t;

using duration_type = int64_t;

constexpr time_type TIME_INFINITE_PAST = INT64_MIN;
constexpr time_type TIME_INFINITE = INT64_MAX;

constexpr static inline time_type time_add_duration(time_type time, duration_type duration)
{
	time_type x = 0;
	if (add_overflow(time, duration, &x)) [[unlikely]]
	{
		if (x >= 0)
		{
			return TIME_INFINITE_PAST;
		}
		else
		{
			return TIME_INFINITE;
		}
	}
	return x;
}

constexpr static inline time_type time_sub_duration(time_type time, duration_type duration)
{
	time_type x = 0;
	if (sub_overflow(time, duration, &x))[[unlikely]]
	{
		if (x >= 0)
		{
			return TIME_INFINITE_PAST;
		}
		else
		{
			return TIME_INFINITE;
		}
	}
	return x;
}

constexpr static inline duration_type time_sub_time(time_type time1, time_type time2)
{
	duration_type x = 0;
	if (sub_overflow(time1, time2, &x))[[unlikely]]
	{
		if (x >= 0)
		{
			return TIME_INFINITE_PAST;
		}
		else
		{
			return TIME_INFINITE;
		}
	}
	return x;
}

constexpr static inline duration_type duration_add_duration(duration_type dur1, duration_type dur2)
{
	duration_type x = 0;
	if (add_overflow(dur1, dur2, &x))[[unlikely]]
	{
		if (x >= 0)
		{
			return TIME_INFINITE_PAST;
		}
		else
		{
			return TIME_INFINITE;
		}
	}
	return x;
}

constexpr static inline duration_type duration_sub_duration(duration_type dur1, duration_type dur2)
{
	duration_type x = 0;
	if (sub_overflow(dur1, dur2, &x))[[unlikely]]
	{
		if (x >= 0)
		{
			return TIME_INFINITE_PAST;
		}
		else
		{
			return TIME_INFINITE;
		}
	}
	return x;
}

constexpr static inline duration_type duration_mul_int64(duration_type duration, int64_t multiplier)
{
	duration_type x = 0;
	if (mul_overflow(duration, multiplier, &x))[[unlikely]]
	{
		if ((duration > 0 && multiplier > 0) || (duration < 0 && multiplier < 0))
		{
			return TIME_INFINITE;
		}
		else
		{
			return TIME_INFINITE_PAST;
		}
	}
	return x;
}