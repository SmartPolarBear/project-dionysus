#pragma once

#include "system/types.h"

#include <compare>

namespace cmos
{
	struct cmos_date_time_struct
	{
		uint16_t second;
		uint16_t minute;

		uint16_t hour;
		bool is_24hour;

		uint16_t day;
		uint16_t month;
		uint16_t year;
		uint16_t century;

		auto operator<=>(const cmos_date_time_struct&) const = default;
	};

	cmos_date_time_struct&& cmos_read_rtc();
}