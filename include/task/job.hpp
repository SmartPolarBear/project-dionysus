#pragma once

#include "object/kernel_object.hpp"

#include "arch/amd64/cpu/regs.h"

#include "system/mmu.h"
#include "system/types.h"
#include "system/vmm.h"
#include "system/syscall.h"
#include "system/messaging.hpp"

#include "drivers/apic/traps.h"
#include "drivers/lock/spinlock.h"

#include "kbl/pod_list.h"
#include "kbl/list_base.hpp"

#include "ktl/mutex/lock_guard.hpp"

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

		[[nodiscard]]std::span<std::optional<policy_item>, POLICY_CONDITION_MAX> get_policies()
		{
			return { action };
		}

		[[nodiscard]] std::optional<policy_item> get_for_condition(policy_condition cond)
		{
			return action[static_cast<size_t>(cond)];
		}

		void apply(policy_item& policy)
		{
			action[static_cast<size_t>(policy.condition)] = policy;
		}

		void merge_with(job_policy&& another)
		{
			for (std::optional<policy_item>& action:another.action)
			{
				if (action.has_value())
				{
					apply(action.value());
				}
			}
		}

	 private:
		std::optional<policy_item> action[POLICY_CONDITION_MAX];
	};

	class job final
		: object::kernel_object
	{
	 public:
		using job_list_type = libkernel::single_linked_child_list_base<job*>;
		using process_list_type = libkernel::single_linked_child_list_base<process_dispatcher*>;
	 public:

	 private:

		lock::spinlock_struct lock;
		lock::spinlock_lockable lockable{ lock };

		job_list_type child_jobs;
		process_list_type processes;

		job_policy policy;

		bool killed;

	};
}