
#include <task/scheduler/ule/per_thread.hpp>

#include "task/scheduler/ule/ule.hpp"

#include "drivers/apic/timer.h"

#include <ktl/algorithm.hpp>

using namespace ktl;

void task::ule_scheduler_state_base::on_tick()
{
	run_time_++;
}

void task::ule_scheduler_state_base::on_sleep()
{
	sleep_tick_ = timer::get_ticks();
}

void task::ule_scheduler_state_base::on_wakeup()
{
	sleep_time_ += timer::get_ticks() - sleep_tick_;
	sleep_tick_ = 0;
}

task::ule_scheduler_state_base::interactivity_score_type task::ule_scheduler_state_base::interactivity_score() const
{
	// if it's not likely to be interactive, avoid expensive computing
	if (interactivity_ <= ule_scheduler_class::INTERACT_HALF
		&& run_time_ >= sleep_time_)
		return ule_scheduler_class::INTERACT_HALF;
	else if (run_time_ == sleep_time_)
		return ule_scheduler_class::INTERACT_HALF;

	if (run_time_ > sleep_time_)
	{
		auto div = max(1ul, run_time_ / ule_scheduler_class::INTERACT_HALF);
		return ule_scheduler_class::INTERACT_HALF +
			(ule_scheduler_class::INTERACT_HALF - sleep_time_ / div);
	}
	else
	{
		auto div = max(1ul, sleep_time_ / ule_scheduler_class::INTERACT_HALF);
		return run_time_ / div;
	}

	KDEBUG_GERNERALPANIC_CODE(-ERROR_SHOULD_NOT_REACH_HERE);
	return 0;
}

void task::ule_scheduler_state_base::update_priority()
{
	priority_ = compute_priority();
}

task::ule_scheduler_state_base::priority_type task::ule_scheduler_state_base::compute_priority()
{
	if (priority_class_ != priority_classes::TIMESHARE)
	{
		return 0;
	}

	auto score = max(0ul, interactivity_ + nice_);
	priority_type ret{ 0 };
	if (score < ule_scheduler_class::INTERACT_THRESHOLD)
	{
		ret = ule_scheduler_class::PRIORITY_MIN_INTERACT;
		ret += ((ule_scheduler_class::PRIORITY_MAX_INTERACT - ule_scheduler_class::PRIORITY_MIN_INTERACT + 1)
			/ ule_scheduler_class::INTERACT_THRESHOLD) * score;
	}
	else{

	}

	return ret;
}
