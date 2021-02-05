#pragma once

#include "system/time.hpp"

#include "debug/kdebug.h"

#include <compare>

enum [[clang::enum_extensibility(closed)]] slack_mode : uint32_t
{
	TIMER_SLACK_CENTER,  // slack is centered around deadline
	TIMER_SLACK_EARLY,    // slack interval is (deadline - slack, deadline]
	TIMER_SLACK_LATE,      // slack interval is [deadline, deadline + slack)
};

class timer_slack final
{
 public:
	[[nodiscard]] constexpr timer_slack(duration_type amount, slack_mode slack) : amount_(amount), slack_(slack)
	{
		KDEBUG_ASSERT(amount_ >= 0);
	}

	[[nodiscard]] static constexpr const timer_slack& none()
	{
		return none_;
	}

	[[nodiscard]] constexpr duration_type amount() const
	{
		return amount_;
	}

	[[nodiscard]] constexpr slack_mode mode() const
	{
		return slack_;
	}

	auto operator<=>(const timer_slack&) const = default;

 private:
	static const timer_slack none_;

	duration_type amount_;
	slack_mode slack_;
};

class deadline final
{
 public:
	[[nodiscard]]constexpr deadline(time_type when, timer_slack slack)
		: when_(when), slack_(slack)
	{
	}

	[[nodiscard]]static constexpr deadline no_slack(time_type when)
	{
		return deadline(when, timer_slack::none());
	}

	[[nodiscard]]static constexpr const deadline& infinite()
	{
		return infinite_;
	}

	[[nodiscard]] constexpr time_type when() const
	{
		return when_;
	}

	[[nodiscard]] constexpr timer_slack slack() const
	{
		return slack_;
	}

	[[nodiscard]]static deadline after(duration_type after, timer_slack slack = timer_slack::none());

	[[nodiscard]]time_type earliest() const;

	[[nodiscard]]time_type latest() const;

 private:
	static const deadline infinite_;

	const time_type when_;
	timer_slack slack_;
};