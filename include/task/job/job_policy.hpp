#pragma once

#include "object/dispatcher.hpp"

#include "arch/amd64/cpu/regs.h"

#include "debug/thread_annotations.hpp"

#include "system/types.h"

#include "kbl/lock/spinlock.h"
#include "kbl/ref_count/ref_count_base.hpp"
#include "kbl/data/name.hpp"

#include "kbl/lock/lock_guard.hpp"
#include "ktl/unique_ptr.hpp"
#include "ktl/list.hpp"
#include "ktl/concepts.hpp"

#include "task/process/process.hpp"

#include <cstring>
#include <algorithm>
#include <span>
#include <optional>

namespace task
{
enum class policy_action
{
	// for the sake of debugging, starting from 1
	kPolicyAllow = 1,
	kPolicyDeny,
	kPolicyKill,
	kPolicyAllowWithException,
	kPolicyDenyWithException,
};

enum class policy_condition : size_t
{
	// TODO: define them
	kPlaceholder,

	kPolicyConditionMax
};

struct policy_item
{
	policy_condition condition;
	policy_action action;
};

class job_policy
{
 public:
	static constexpr size_t POLICY_CONDITION_MAX = static_cast<size_t >(policy_condition::kPolicyConditionMax);

 public:
	job_policy(const job_policy& parent) = default;

	static job_policy creat_root_policy()
	{
		//TODO
		return job_policy();
	}

	[[nodiscard]]std::span<std::optional<policy_item>, POLICY_CONDITION_MAX> get_policies()
	{
		return { action_ };
	}

	[[nodiscard]] std::optional<policy_item> query(policy_condition cond)
	{
		return action_[static_cast<size_t>(cond)];
	}

	void apply(policy_item& policy)
	{
		action_[static_cast<size_t>(policy.condition)] = policy;
	}

	void merge_with(job_policy&& another)
	{
		for (std::optional<policy_item>& ac:another.action_)
		{
			if (ac.has_value())
			{
				apply(ac.value());
			}
		}
	}

 private:
	job_policy() = default;

	std::optional<policy_item> action_[POLICY_CONDITION_MAX]{};
};

}