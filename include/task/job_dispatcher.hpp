#pragma once

#include "object/kernel_object.hpp"

#include "arch/amd64/cpu/regs.h"

#include "system/mmu.h"
#include "system/types.h"
#include "system/vmm.h"
#include "system/syscall.h"
#include "system/messaging.hpp"

#include "drivers/apic/traps.h"
#include "kbl/lock/spinlock.h"

#include "kbl/data/pod_list.h"
#include "kbl/data/list_base.hpp"

#include "ktl/mutex/lock_guard.hpp"
#include "kbl/atomic/atomic_ref.hpp"
#include "kbl/ref_count/ref_count_base.hpp"

#include "task/process_dispatcher.hpp"
#include "task/dispatcher.hpp"

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

	class job_dispatcher;

	class job_enumerator
	{
	 public:
		virtual bool on_job(job_dispatcher* jb)
		{
			return true;
		}

		virtual bool on_process(process_dispatcher* pr)
		{
			return true;
		}
	 protected:
		virtual ~job_enumerator() = default;
	};

	enum class job_status
	{
		JS_READY,
		JS_KILLING,
		JS_DEAD
	};

	class job_dispatcher final
		: public dispatcher<job_dispatcher, 0>
	{
	 public:
		using job_list_type = libkernel::single_linked_child_list_base<job_dispatcher*>;
		using process_list_type = libkernel::single_linked_child_list_base<process_dispatcher*>;
		static constexpr size_t JOB_NAME_MAX = 64;
	 public:
		[[nodiscard]]bool kill(error_code terminate_code) noexcept;

		[[nodiscard]]error_code apply_basic_policy(uint64_t mode, std::span<policy_item> policies) noexcept;

		[[nodiscard]]job_policy&& get_policy() const;

		[[nodiscard]]error_code enumerate_children(job_enumerator* enumerator, bool recurse);

		static std::unique_ptr<job_dispatcher> create_root();

		static error_code_with_result<job_dispatcher> create(uint64_t flags,
			std::shared_ptr<job_dispatcher> parent,
			right_type right);

		~job_dispatcher() final;
	 private:
		job_list_type child_jobs;
		process_list_type processes;

		job_policy policy;

		std::shared_ptr<job_dispatcher> parent;

		char name[JOB_NAME_MAX];

		bool killed;

		job_status status;

	};
}