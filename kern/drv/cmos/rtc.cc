#include "arch/amd64/cpu/x86.h"

#include "drivers/cmos/rtc.hpp"
#include "drivers/acpi/acpi.h"

#include "system/error.hpp"
#include "system/mmu.h"
#include "system/pmm.h"
#include "system/segmentation.hpp"
#include "system/vmm.h"

#include "../../libs/basic_io/include/builtin_text_io.hpp"
#include <cstring>

using namespace cmos;

constexpr uint16_t CMOS_ADDR = 0x70;
constexpr uint16_t CMOS_DATA = 0x71;

constexpr uint16_t bcd_to_binary(uint16_t bcd)
{
	return ((bcd & 0xF0u) >> 1u) + ((bcd & 0xF0u) >> 3u) + (bcd & 0xfu);
}

using rtc_reg_type = uint8_t;

rtc_reg_type century_reg = UINT8_MAX;
bool is_bissextile = false;

// Preprocessed in the initialization method
uint64_t bissextile_month_ps_arr[13] = { 0, 0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30 };
uint64_t normal_month_ps_arr[13] = { 0, 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30 };

uint64_t* current_month_ps_arr = nullptr;

cmos_date_time_struct boot_time{};

enum rtc_regs : rtc_reg_type
{
	RTC_REG_SECOND = 0x00,
	RTC_REG_MINUTE = 0x02,
	RTC_REG_HOUR = 0x04,
	RTC_REG_WEEKDAY = 0x06,
	RTC_REG_DAY_OF_MONTH = 0x07,
	RTC_REG_MONTH = 0x08,
	RTC_REG_YEAR = 0x09,
};

struct byte_format_value
{
	uint64_t reserved0: 1;
	uint64_t enable_24h: 1;
	uint64_t enable_bin_mode: 1;
	uint64_t reserved1: 1;
}__attribute__((packed));

static inline uint64_t get_day_of_year(uint64_t mon, uint64_t day)
{
	return current_month_ps_arr[mon] + day;
}

static inline bool is_updating()
{
	outb(CMOS_ADDR, 0x0A);
	return inb(CMOS_DATA) & 0x80;
}

static inline uint16_t rtc_register_read(rtc_reg_type reg)
{
	outb(CMOS_ADDR, reg);
	return inb(CMOS_DATA);
}

static constexpr bool check_bissextile(uint64_t year)
{
	return ((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0);
}

static inline void wait_for_update()
{
	while (is_updating()); // Do nothing
}

static inline void fill_date_time_struct(OUT cmos_date_time_struct& date_time)
{
	date_time.second = rtc_register_read(RTC_REG_SECOND);
	date_time.minute = rtc_register_read(RTC_REG_MINUTE);
	date_time.hour = rtc_register_read(RTC_REG_HOUR);
	date_time.weekday = rtc_register_read(RTC_REG_WEEKDAY);
	date_time.day_of_month = rtc_register_read(RTC_REG_DAY_OF_MONTH);
	date_time.month = rtc_register_read(RTC_REG_MONTH);
	date_time.year = rtc_register_read(RTC_REG_YEAR);

	if (century_reg != UINT8_MAX)
	{
		date_time.century = rtc_register_read(century_reg);
	}

}

static inline void parse_bcd_values(OUT cmos_date_time_struct& date_time)
{
	date_time.second = bcd_to_binary(date_time.second);
	date_time.minute = bcd_to_binary(date_time.minute);
	date_time.hour = bcd_to_binary(date_time.hour);
	date_time.weekday = bcd_to_binary(date_time.weekday);
	date_time.day_of_month = bcd_to_binary(date_time.day_of_month);
	date_time.month = bcd_to_binary(date_time.month);
	date_time.year = bcd_to_binary(date_time.year);

	if (century_reg != UINT8_MAX)
	{
		date_time.century = bcd_to_binary(date_time.century);
	}

}

cmos::cmos_date_time_struct&& cmos::cmos_read_rtc()
{

	cmos_date_time_struct previous{};
	cmos_date_time_struct now{};

	wait_for_update();

	fill_date_time_struct(now);

	do
	{
		previous = now;

		wait_for_update();

		fill_date_time_struct(now);
	}
	while (now != previous);

	auto byte_format_raw = rtc_register_read(0x0B);
	auto byte_format = *reinterpret_cast<byte_format_value*>(&byte_format_raw);

	if (!byte_format.enable_bin_mode) // BCD mode
	{
		parse_bcd_values(now);
	}

	/* From osdev.org:
	 *
	 * 12 hour time is annoying to convert back to 24 hour time.
	 * If the hour is pm, then the 0x80 bit is set on the hour byte.
	 * So you need to mask that off. (This is true for both binary and BCD modes.)
	 * Then, midnight is 12, 1am is 1, etc.
	 * Note that carefully: midnight is not 0
	 * -- it is 12 --
	 * this needs to be handled as a special case in the calculation
	 * from 12 hour format to 24 hour format (by setting 12 back to 0)!*/

	if (!byte_format.enable_24h)
	{
		now.hour = ((now.hour & 0x7F) + 12) % 24;
	}

	now.real_year = now.year;


	// Refine the year value
	if (century_reg == UINT8_MAX)
	{
		// Guess the century
		if (now.real_year < 90) // We just can't have read a time of 2090s, so it's 21 century
		{
			now.real_year = 2000 + now.real_year;
		}
		else if (now.real_year > 90) // It is probably 1990s
		{
			now.real_year = 1900 + now.real_year;
		}
	}
	else
	{
		now.real_year = now.century * 100 + now.real_year;
	}

	is_bissextile = check_bissextile(now.real_year);
	if (is_bissextile)
	{
		current_month_ps_arr = bissextile_month_ps_arr;
	}
	else
	{
		current_month_ps_arr = normal_month_ps_arr;
	}

	return std::move(now);
}

PANIC void cmos::cmos_rtc_init()
{
	auto fadt = acpi::get_fadt();

	if (fadt == nullptr)
	{
		KDEBUG_GENERALPANIC("FADT isn't exist.");
	}

	if (fadt->century != 0)
	{
		century_reg = fadt->century;
	}

	// preprocessing the prefix sums
	for (int i = 1; i <= 12; ++i)
	{
		bissextile_month_ps_arr[i] = bissextile_month_ps_arr[i - 1] + bissextile_month_ps_arr[i];
		normal_month_ps_arr[i] = normal_month_ps_arr[i - 1] + normal_month_ps_arr[i];
	}

	boot_time = cmos_read_rtc();

	kdebug::kdebug_log("Boot up time: %lld-%lld-%lld %lld:%lld:%lld, timestamp %lld\n",
		boot_time.real_year, boot_time.month, boot_time.day_of_month,
		boot_time.hour, boot_time.minute, boot_time.second,
		datetime_to_timestamp(boot_time));
}

timestamp_t cmos::datetime_to_timestamp(const cmos_date_time_struct& datetime)
{
	/* POSIX says:
	 *
	 * A value that approximates the number of seconds that have elapsed since the Epoch. A Coordinated Universal Time name (specified in terms of seconds (tm_sec), minutes (tm_min), hours (tm_hour), days since January 1 of the year (tm_yday), and calendar year minus 1900 (tm_year)) is related to a time represented as seconds since the Epoch, according to the expression below.
	If the year is <1970 or the value is negative, the relationship is undefined. If the year is >=1970 and the value is non-negative, the value is related to a Coordinated Universal Time name according to the C-language expression, where tm_sec, tm_min, tm_hour, tm_yday, and tm_year are all integer types:
	tm_sec + tm_min*60 + tm_hour*3600 + tm_yday*86400 +
    (tm_year-70)*31536000 + ((tm_year-69)/4)*86400 -
    ((tm_year-1)/100)*86400 + ((tm_year+299)/400)*86400
    */

	auto year = datetime.real_year - 1900;

	return datetime.second + datetime.minute * 60ull + datetime.hour * 3600ull
		+ get_day_of_year(datetime.month, datetime.day_of_month) * 86400ull +
		(year - 70ull) * 31536000ull + ((year - 69ull) / 4ull) * 86400ull -
		((year - 1ull) / 100ull) * 86400ull + ((year + 299ull) / 400ull) * 86400ull;
}

timestamp_t cmos::cmos_read_rtc_timestamp()
{
	auto datetime = cmos_read_rtc();
	return datetime_to_timestamp(datetime);
}

timestamp_t cmos::get_boot_timestamp()
{
	return datetime_to_timestamp(boot_time);
}

cmos_date_time_struct&& cmos::get_boot_time()
{
	return std::move(boot_time);
}
