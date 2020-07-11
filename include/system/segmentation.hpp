#pragma once

#include "system/concepts.hpp"
#include "system/types.h"

#include "drivers/lock/spinlock.h"

struct pseudo_descriptor
{
	uint16_t limit;
	uint64_t base;
} __attribute__((__packed__));

using gdt_table_desc = pseudo_descriptor;

struct idt_entry
{
	uint32_t gd_off_15_0: 16;  // [0 ~ 16] bits of offset in segment
	uint32_t gd_ss: 16;        // segment selector
	uint32_t gd_ist: 3;        // interrupt stack table
	uint32_t gd_rsv1: 5;       // reserved bits
	uint32_t gd_type: 4;       // type (STS_{TG,IG32,TG32})
	uint32_t gd_s: 1;          // 0 for system, 1 for code or data
	uint32_t gd_dpl: 2;        // descriptor(meaning new) privilege level
	uint32_t gd_p: 1;          // Present
	uint32_t gd_off_31_16: 16; // [16 ~ 31] bits of offset in segment
	uint32_t gd_off_63_32: 32; // [32 ~ 63] bits of offset in segment
	uint32_t gd_rsv2: 32;      // reserved bits
}__attribute__((__packed__));

struct gdt_access_byte_struct
{
	uint64_t ac: 1;
	uint64_t rw: 1;
	uint64_t dc: 1;
	uint64_t ex: 1;
	uint64_t s: 1;
	uint64_t privl: 2;
	uint64_t pr: 1;
}__attribute__((__packed__));

struct gdt_flags_struct
{
	uint64_t always0_1: 1;
	uint64_t l: 1;
	uint64_t sz: 1;
	uint64_t gr: 1;
}__attribute__((__packed__));

static inline uint8_t gdt_flags_to_int(gdt_flags_struct flags)
{
	uint8_t access_byte = *((uint8_t*)(&flags));
	return access_byte;
}

static inline uint8_t gdt_access_byte_to_int(gdt_access_byte_struct ab)
{
	uint8_t flags = *((uint8_t*)(&ab));
	return flags;
}

struct gdt_entry
{
	uint64_t limit_low: 16;
	uint64_t base_low: 16;
	uint64_t base_mid: 8;
	uint64_t access_byte: 8;
	uint64_t limit_high: 4;
	uint64_t flags: 4;
	uint64_t base_high: 8;
} __attribute__((__packed__));


using idt_table_desc = pseudo_descriptor;

struct task_state_segment
{
	uint32_t reserved0;
	uint64_t rsp0;
	uint64_t rsp1;
	uint64_t rsp2;
	uint64_t reserved1;
	uint64_t ist1;
	uint64_t ist2;
	uint64_t ist3;
	uint64_t ist4;
	uint64_t ist5;
	uint64_t ist6;
	uint64_t ist7;
	uint64_t reserved2;
	uint16_t reserved3;
	uint16_t iopb_offset;
} __attribute__((__packed__));

struct gdt_table
{
	struct gdt_entry null;
	struct gdt_entry kernel_code;
	struct gdt_entry kernel_data;
	struct gdt_entry null2;
	struct gdt_entry user_data;
	struct gdt_entry user_code;
	struct gdt_entry ovmf_data;
	struct gdt_entry ovmf_code;
	struct gdt_entry tss_low;
	struct gdt_entry tss_high;
} __attribute__((aligned(4096)));

enum GDT_SEGMENTS : uint64_t
{
	SEGMENTSEL_KNULL = 0x00,
	SEGMENTSEL_KCODE = 0x08,
	SEGMENTSEL_KDATA = 0x10,
	SEGMENTSEL_UNULL = 0x18,
	SEGMENTSEL_UDATA = 0x20,
	SEGMENTSEL_UCODE = 0x28,
	SEGMENTSEL_OVMFDATA = 0x30,
	SEGMENTSEL_OVMFCODE = 0x38,
	SEGMENTSEL_TSSLOW = 0x40,
	SEGMENTSEL_TSSHIGH = 0x48
};

enum SEGMENT_TYPE
{
	STA_X = 0x8,                 // Executable segment
	STA_E = 0x4,                 // Expand down (non-executable segments)
	STA_C = 0x4,                 // Conforming code segment (executable only)
	STA_W = 0x2,                 // Writeable (non-executable segments)
	STA_R = 0x2,                 // Readable (executable segments)
	STA_A = 0x1,                 // Accessed
	STS_T16A = 0x1,                 // Available 16-bit TSS
	STS_LDT = 0x2,                 // Local Descriptor Table
	STS_T16B = 0x3,                 // Busy 16-bit TSS
	STS_CG16 = 0x4,                 // 16-bit Call Gate
	STS_TG = 0x5,                 // Task Gate / Coum Transmitions
	STS_IG16 = 0x6,                 // 16-bit Interrupt Gate
	STS_TG16 = 0x7,                 // 16-bit Trap Gate
	STS_T32A = 0x9,                 // Available 32-bit TSS
	STS_T32B = 0xB,                 // Busy 32-bit TSS
	STS_CG32 = 0xC,                 // 32-bit Call Gate
	STS_IG32 = 0xE,                 // 32-bit Interrupt Gate
	STS_TG32 = 0xF,                 // 32-bit Trap Gate
};
// cpu local storage

static_assert(sizeof(uintptr_t) == 0x08);
enum CLS_ADDRESS : uintptr_t
{
	CLS_CPU_STRUCT_PTR = 0,
	CLS_PROC_STRUCT_PTR = 0x8
};

#pragma clang diagnostic push

// bypass the problem caused by the fault of clang
// that parameters used in inline asm are always reported to be unused

#pragma clang diagnostic ignored "-Wunused-variable"
#pragma clang diagnostic ignored "-Wunused-parameter"

//__attribute__((always_inline))

template<typename T>
static inline T cls_get(uintptr_t n)
requires Pointer<T>
{
	uintptr_t ret = 0;
	asm("mov %%fs:(%%rax),%0"
	: "=r"(ret)
	: "a"(n));
	return (T)(void*)ret;
}

template<typename T>
static inline void cls_put(uintptr_t n, T v)
requires Pointer<T>
{
	uintptr_t val = (uintptr_t)v;
	asm("mov %0, %%fs:(%%rax)"
	:
	: "r"(val), "a"(n));
}

template<typename T>
static inline T gs_get(uintptr_t n)
requires Pointer<T>
{
	uintptr_t ret = 0;
	asm("mov %%gs:(%%rax),%0"
	: "=r"(ret)
	: "a"(n));
	return (T)(void*)ret;
}

template<typename T>
static inline void gs_put(uintptr_t n, T v)
requires Pointer<T>
{
	uintptr_t val = (uintptr_t)v;
	asm("mov %0, %%gs:(%%rax)"
	:
	: "r"(val), "a"(n));
}

#pragma clang diagnostic pop

template<typename T, CLS_ADDRESS addr>
requires Pointer<T>
class CLSItem
{
 private:
	lock::spinlock lk;
	bool use_lock;
 public:
	CLSItem() : use_lock(true)
	{
		lock::spinlock_initlock(&lk, "clslock");
	}

	CLSItem(bool _use_lock) : use_lock(_use_lock)
	{
		lock::spinlock_initlock(&lk, "clslock");
	}

	void set_lock(bool _use_lock)
	{
		use_lock = _use_lock;
	}

	bool get_lock()
	{
		return use_lock;
	}

	T operator()()
	{
		return cls_get<T>(addr);
	}

	T operator->()
	{
		return cls_get<T>(addr);
	}

	void operator=(T src)
	{
		if (use_lock)
		{
			lock::spinlock_acquire(&this->lk);
		}

		cls_put(addr, src);

		if (use_lock)
		{
			lock::spinlock_release(&this->lk);
		}
	}

};

template<typename T, CLS_ADDRESS addr>
requires Pointer<T>
static inline bool operator==(CLSItem<T, addr> lhs, CLSItem<T, addr> rhs)
{
	return lhs() == rhs();
}

template<typename T, CLS_ADDRESS addr>
requires Pointer<T>
static inline bool operator!=(CLSItem<T, addr> lhs, CLSItem<T, addr> rhs)
{
	return !(lhs() == rhs());
}

template<typename T, CLS_ADDRESS addr>
requires Pointer<T>
static inline bool operator==(T lhs, CLSItem<T, addr> rhs)
{
	return lhs == rhs();
}

template<typename T, CLS_ADDRESS addr>
requires Pointer<T>
static inline bool operator!=(T lhs, CLSItem<T, addr> rhs)
{
	return !(lhs == rhs());
}

template<typename T, CLS_ADDRESS addr>
requires Pointer<T>
static inline bool operator==(CLSItem<T, addr> lhs, T rhs)
{
	return lhs() == rhs;
}

template<typename T, CLS_ADDRESS addr>
requires Pointer<T>
static inline bool operator!=(CLSItem<T, addr> lhs, T rhs)
{
	return !(lhs() == rhs);
}

template<typename T, CLS_ADDRESS addr>
requires Pointer<T>
static inline bool operator==(CLSItem<T, addr> lhs, [[maybe_unused]] nullptr_t rhs)
{
	return lhs() == nullptr;
}

template<typename T, CLS_ADDRESS addr>
requires Pointer<T>
static inline bool operator!=(CLSItem<T, addr> lhs, [[maybe_unused]]nullptr_t rhs)
{
	return !(lhs() == nullptr);
}

template<typename T, CLS_ADDRESS addr>
requires Pointer<T>
static inline bool operator==([[maybe_unused]]nullptr_t lhs, CLSItem<T, addr> rhs)
{
	return nullptr == rhs();
}

template<typename T, CLS_ADDRESS addr>
requires Pointer<T>
static inline bool operator!=([[maybe_unused]]nullptr_t lhs, CLSItem<T, addr> rhs)
{
	return !(nullptr == rhs());
}

