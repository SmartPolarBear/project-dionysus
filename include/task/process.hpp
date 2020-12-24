#pragma once
#include "task/task_dispatcher.hpp"

namespace task
{
	class process_dispatcher
		: public task_dispatcher
	{
	 public:
		process_dispatcher(std::span<char> name, process_id id, process_id parent_id, size_t flags);
		~process_dispatcher() override = default;
		error_code initialize() override;
		error_code setup_mm() override;
		error_code load_binary(uint8_t* bin, size_t binary_size, binary_types type, size_t flags) override;
		error_code terminate(error_code terminate_error) override;
		error_code exit() override;
		error_code sleep(sleep_channel_type channel, lock::spinlock* lk) override;
		error_code wakeup(sleep_channel_type channel) override;
		error_code wakeup_no_lock(sleep_channel_type channel) override;
		error_code change_heap_ptr(uintptr_t* heap_ptr) override;

	 private:
		error_code setup_kernel_stack();
		error_code setup_registers();
	};
}