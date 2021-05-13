#pragma once

#include "system/memlayout.h"
#include "system/error.hpp"
#include "system/vmm.h"

#include "kbl/singleton/singleton.hpp"

#include "memory/fpage.hpp"
#include "memory/pmm_provider.hpp"
#include "memory/buddy_provider.hpp"

namespace memory
{

class physical_memory_manager final
	// we do not use singleton because it overwrite the ap_boot
	//	: public kbl::singleton<physical_memory_manager>
{
 public:
	using provider_type = buddy_provider;

	physical_memory_manager(const physical_memory_manager&) = delete;
	physical_memory_manager& operator=(const physical_memory_manager&) = delete;

	static physical_memory_manager* instance();
 public:

	physical_memory_manager();

	[[nodiscard]] bool is_well_constructed() const;

	void setup_for_base(page* base, size_t n);

	[[nodiscard]] void* asserted_allocate();

	[[nodiscard]] page* allocate();
	[[nodiscard]] page* allocate(size_t n);
	[[nodiscard]] error_code_with_result<page*> allocate(uintptr_t addr,
		size_t n,
		uint64_t perm,
		vmm::pde_ptr_t pgdir,
		bool rewrite_if_exist);
	[[nodiscard]] error_code_with_result<page*> allocate(uintptr_t addr,
		uint64_t perm,
		vmm::pde_ptr_t pgdir,
		bool rewrite_if_exist);

	void free(page* base);
	void free(page* base, size_t n);

	[[nodiscard]] size_t free_count() const;

	error_code insert_page(page* page, uintptr_t va, uint64_t perm, vmm::pde_ptr_t pgdir, bool allow_rewrite);
	void remove_page(uintptr_t va, vmm::pde_ptr_t pgdir);

	// TODO: this should belong to VMM
	void flush_tlb(vmm::pde_ptr_t pgdir, uintptr_t va);

 private:

	void remove_from_pgdir(vmm::pde_ptr_t pde, vmm::pde_ptr_t pgdir, uintptr_t va);

	page* allocate_locked(size_t n);

	provider_type provider_{};
	mutable lock::spinlock lock_{ "pmm" };
};

}