#include "arch/amd64/x86.h"

#include "drivers/cmos/rtc.hpp"
#include "drivers/acpi/acpi.h"

#include "system/error.hpp"
#include "system/mmu.h"
#include "system/pmm.h"
#include "system/segmentation.hpp"
#include "system/vmm.h"

#include "libkernel/console/builtin_text_io.hpp"
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

enum rtc_regs : rtc_reg_type
{
	RTC_REG_SECOND = 0x00,
	RTC_REG_MINUTE = 0x02,
	RTC_REG_HOUR = 0x04,
	RTC_REG_DAY = 0x07,
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

static inline void wait_for_update()
{
	while (is_updating()); // Do nothing
}

static inline void fill_date_time_struct(OUT cmos_date_time_struct& date_time)
{
	date_time.second = rtc_register_read(RTC_REG_SECOND);
	date_time.minute = rtc_register_read(RTC_REG_MINUTE);
	date_time.hour = rtc_register_read(RTC_REG_HOUR);
	date_time.day = rtc_register_read(RTC_REG_DAY);
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
	date_time.day = bcd_to_binary(date_time.day);
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

	now.is_24hour = byte_format.enable_24h;

	if (!byte_format.enable_bin_mode) // BCD mode
	{
		parse_bcd_values(now);
	}

	return std::move(now);
}

PANIC void cmos::cmos_rtc_init()
{
	auto fadt = acpi::get_fadt();
	if (fadt->century != 0)
	{
		century_reg = fadt->century;
	}
}
