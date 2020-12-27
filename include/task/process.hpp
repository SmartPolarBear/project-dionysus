#pragma once
#include "task/task_dispatcher.hpp"

//FIXME

#include "system/scheduler.h"

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
		error_code sleep(sleep_channel_type channel, lock::spinlock_struct* lk) override;
		error_code wakeup(sleep_channel_type channel) override;
		error_code wakeup_no_lock(sleep_channel_type channel) override;
		error_code change_heap_ptr(uintptr_t* heap_ptr) override;

	 private:
		error_code setup_kernel_stack();
		error_code setup_registers();

		//FIXME
	 public:
		friend error_code create_process(IN const char* name,
			IN size_t flags,
			IN bool inherit_parent,
			OUT process_dispatcher** ret);

		friend error_code process_load_binary(IN process_dispatcher* porc,
			IN uint8_t* bin,
			IN size_t binary_size,
			IN binary_types type,
			IN size_t flags);

		// handle task cleanup when exiting
		friend void process_exit(IN process_dispatcher* proc);

		// terminate current task
		friend error_code process_terminate(error_code error_code);

		// sleep on certain channel
		friend error_code process_sleep(size_t channel, lock::spinlock_struct* lock);

		// wake up processes sleeping on certain channel
		friend error_code process_wakeup(size_t channel);
		friend error_code process_wakeup_nolock(size_t channel);

		// send and receive message
		friend error_code process_ipc_send(process_id pid, IN const void* message, size_t size);
		friend error_code process_ipc_receive(OUT void* message_out);
		// send and receive a page
		friend error_code process_ipc_send_page(process_id pid, uint64_t unique_val, IN const void* page, size_t perm);
		friend error_code process_ipc_receive_page(OUT void* out_page);

		// allocate more memory
		friend error_code process_heap_change_size(IN process_dispatcher* proc, IN OUT uintptr_t* heap_ptr);

		friend size_t process_terminate_impl(task::process_dispatcher* proc,
			error_code err);

		friend error_code alloc_ustack(task::process_dispatcher* proc);

		friend void scheduler::scheduler_enter();

		friend void scheduler::scheduler_loop();

		friend void scheduler::scheduler_yield();
	};
}