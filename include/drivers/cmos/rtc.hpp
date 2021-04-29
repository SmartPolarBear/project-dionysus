#pragma once

#include "system/types.h"
#include "system/time.hpp"

#include <compare>

namespace cmos
{
	struct cmos_date_time_struct
	{
		uint64_t second;
		uint64_t minute;
		uint64_t hour;
		uint64_t weekday;
		uint64_t day_of_month;
		uint64_t month;
		uint64_t year;
		uint64_t real_year;
		uint64_t century;

		auto operator<=>(const cmos_date_time_struct&) const = default;
	};

	PANIC void cmos_rtc_init();

	cmos_date_time_struct&& cmos_read_rtc();
	timestamp_type cmos_read_rtc_timestamp();
	timestamp_type datetime_to_timestamp(const cmos_date_time_struct& datetime);

	cmos_date_time_struct get_boot_time();
	timestamp_type get_boot_timestamp();
}