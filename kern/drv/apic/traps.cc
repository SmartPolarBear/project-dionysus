#include "./traps.hpp"
#include "./exception.hpp"

#include "arch/amd64/cpu/x86.h"

#include "task/thread/thread.hpp"
#include "task/scheduler/scheduler.hpp"

#include "drivers/apic/apic.h"
#include "drivers/apic/traps.h"
#include "drivers/console/console.h"
#include "debug/kdebug.h"
#include "kbl/lock/spinlock.h"

#include "system/error.hpp"
#include "system/mmu.h"
#include "system/pmm.h"
#include "system/segmentation.hpp"
#include "system/vmm.h"

#include "task/process/process.hpp"

#include "../../libs/basic_io/include/builtin_text_io.hpp"
#include <cstring>

using trap::trap_handle;
using trap::TRAP_NUMBERMAX;

using lock::spinlock_struct;
using lock::spinlock_acquire;
using lock::spinlock_initialize_lock;
using lock::spinlock_release;

constexpr size_t IDT_SIZE = 4_KB;

#pragma clang diagnostic push

#pragma clang diagnostic ignored "-Wc99-designator"
#pragma clang diagnostic ignored "-Winitializer-overrides"

struct handle_table_struct
{
	lock::spinlock_struct lock{};

	trap::trap_handle trap_handles[trap::TRAP_NUMBERMAX] =
		{
			[0 ... trap::TRAP_NUMBERMAX - 1] = trap::trap_handle{
				.handle = default_trap_handle,
				.enable = true
			},

			[trap::TRAP_MSI_BASE]=trap::trap_handle{
				.handle = msi_base_trap_handle,
				.enable = true
			},
		};
} handle_table;

#pragma clang diagnostic pop;

// default handle of trap
error_code default_trap_handle([[maybe_unused]] trap::trap_frame info)
{
	// the handle doesn't exist

	write_format("trap %d caused but not handled.\n",
		info.trap_num);

	trap::print_trap_frame(&info);

	return ERROR_SUCCESS;
}

static error_code spurious_trap_handle([[maybe_unused]] trap::trap_frame info)
{

	return ERROR_SUCCESS;
}

extern "C" void* vectors[]; // generated by gvectors.py, compiled from apic/vectors.S

// defined below. used by trap_entry to futuer task a trap
extern "C" void trap_body(trap::trap_frame info);

static inline void make_idt_entry(idt_entry* gate, exception_type type, uintptr_t sel, size_t off, size_t dpl)
{
	gate->gd_off_15_0 = (uint64_t)(off) & 0xffffull;
	gate->gd_ss = (sel);
	gate->gd_ist = 0;
	gate->gd_rsv1 = 0;
	gate->gd_type = (uint8_t)type;
	gate->gd_s = 0;
	gate->gd_dpl = (dpl);
	gate->gd_p = 1;
	gate->gd_off_31_16 = (uint64_t)(off) >> 16ull;
	gate->gd_off_63_32 = (uint64_t)(off) >> 32ull;
	gate->gd_rsv2 = 0;
}

PANIC void trap::init_trap()
{
	auto idt = reinterpret_cast<idt_entry*>(new(std::nothrow)BLOCK<IDT_SIZE>);
	if (idt == nullptr)
	{
		KDEBUG_GENERALPANIC("Can't allocate memory for IDT.");
	}
	memset(idt, 0, IDT_SIZE);

	auto desc = reinterpret_cast<idt_table_desc*>(new(std::nothrow)BLOCK<sizeof(idt_table_desc)>);
	if (desc == nullptr)
	{
		KDEBUG_GENERALPANIC("Can't allocate memory for IDT descriptor.");
	}
	memset(desc, 0, sizeof(*desc));

	// fill the descriptor
	desc->limit = IDT_SIZE - 1;
	desc->base = (uintptr_t)idt;

	// fill the idt table
	for (size_t i = 0; i < 256; i++)
	{
		// we task MSI interrupts on boot CPU core.
		make_idt_entry(&idt[i], IT_INTERRUPT, SEGMENTSEL_KCODE, (uintptr_t)vectors[i], DPL_KERNEL);
	}

	load_idt(desc);

	spinlock_initialize_lock(&handle_table.lock, "traphandles");

	trap_handle_register(trap::irq_to_trap_number(IRQ_SPURIOUS), trap_handle{
		.handle = spurious_trap_handle,
		.enable = true
	});

	install_exception_handles();
}

// returns the old handle
PANIC trap_handle trap::trap_handle_register(size_t trapnumber, trap_handle handle)
{
	spinlock_acquire(&handle_table.lock);

	trap_handle old = trap_handle{ handle_table.trap_handles[trapnumber] };
	handle_table.trap_handles[trapnumber] = handle;

	spinlock_release(&handle_table.lock);

	return old;
}

PANIC bool trap::trap_handle_enable(size_t number, bool enable)
{
	spinlock_acquire(&handle_table.lock);

	bool old = handle_table.trap_handles[number].enable;
	handle_table.trap_handles[number].enable = enable;

	spinlock_release(&handle_table.lock);

	return old;
}

extern "C" void trap_body(trap::trap_frame info)
{
	// check if the trap number is out of range
	if (info.trap_num >= TRAP_NUMBERMAX || info.trap_num < 0)
	{
		KDEBUG_RICHPANIC("trap number is out of range", "KERNEL PANIC: TRAP", false, "The given trap number is %d",
			info.trap_num);
	}

	// it should be assigned with the default handle when initialized
	KDEBUG_ASSERT(handle_table.trap_handles[info.trap_num].handle != nullptr);

	error_code error = ERROR_SUCCESS;

	// call the handle
	if (handle_table.trap_handles[info.trap_num].enable)
	{
		error = handle_table.trap_handles[info.trap_num].handle(info);
	}
	else
	{
		error = default_trap_handle(info);
	}

	// finish the trap handle
	local_apic::write_eoi();

	// If in the user space, directly kill it
	if (cur_proc.get() != nullptr
		&& (cur_proc->get_flags() & task::PROC_EXITING)
		&& ((info.cs & 0b11) == DPL_USER))
	{
		//FIXME
		KDEBUG_NOT_IMPLEMENTED;
	}

	// if rescheduling needed, reschedule
	if (task::cur_thread != nullptr && task::cur_thread->get_need_reschedule())
	{
		cpu->scheduler.yield();
	}

	if (error != ERROR_SUCCESS)
	{
		KDEBUG_RICHPANIC_CODE(error, true, "");
	}
}

void trap::print_trap_frame(IN const trap_frame* p_tf)
{
	write_format("Trap frame content at 0x%p : \n", p_tf);

	write_format("  rax=0x%x", p_tf->rax);
	write_format("  rbx=0x%x", p_tf->rbx);
	write_format("  rcx=0x%x", p_tf->rcx);
	write_format("  rdx=0x%x", p_tf->rdx);
	write_format("  rbp=0x%x\n", p_tf->rbp);
	write_format("  rsi=0x%x", p_tf->rsi);
	write_format("  rdi=0x%x", p_tf->rdi);
	write_format("  r8=0x%x", p_tf->r8);
	write_format("  r9=0x%x", p_tf->r9);
	write_format("  r10=0x%x\n", p_tf->r10);
	write_format("  r11=0x%x", p_tf->r11);
	write_format("  r12=0x%x", p_tf->r12);
	write_format("  r13=0x%x", p_tf->r13);
	write_format("  r14=0x%x", p_tf->r14);
	write_format("  r15=0x%x\n", p_tf->r15);
	write_format("  trap number=0x%x", p_tf->trap_num);
	write_format("  err=0x%x", p_tf->err);
	write_format("  rip=0x%x", p_tf->rip);
	write_format("  cs=0x%x", p_tf->cs);
	write_format("  rflags=0x%x\n", p_tf->rflags);
	write_format("  rsp=0x%x", p_tf->rsp);
	write_format("  ss=0x%x\n", p_tf->ss);

	write_format("End trap frame content at 0x%p .\n", p_tf);
}
