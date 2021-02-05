#include "system/deadline.hpp"

#include "drivers/cmos/rtc.hpp"

#include "debug/kdebug.h"

const timer_slack timer_slack::none_{ 0, TIMER_SLACK_CENTER };

const deadline deadline::infinite_{ TIME_INFINITE, timer_slack::none() };

deadline deadline::after(duration_type after, timer_slack slack)
{
	auto timestamp = time_add_duration(cmos::cmos_read_rtc_timestamp(), after);
	return deadline(timestamp, slack);
}

time_type deadline::latest() const
{
	switch (slack_.mode())
	{
	case TIMER_SLACK_CENTER:
		return time_sub_duration(when_, slack_.amount());
	case TIMER_SLACK_LATE:
		return when_;
	case TIMER_SLACK_EARLY:
		return time_sub_duration(when_, slack_.amount());
	default:
		KDEBUG_RICHPANIC("invalid timer mode\n", "Deadline", false, "slack mode :%u\n", slack_.mode());
	}
}

time_type deadline::earliest() const
{
	switch (slack_.mode())
	{
	case TIMER_SLACK_CENTER:
		return time_sub_duration(when_, slack_.amount());
	case TIMER_SLACK_LATE:
		return when_;
	case TIMER_SLACK_EARLY:
		return time_sub_duration(when_, slack_.amount());
	default:
		KDEBUG_RICHPANIC("invalid timer mode\n", "Deadline", false, "slack mode :%u\n", slack_.mode());
	}
}
