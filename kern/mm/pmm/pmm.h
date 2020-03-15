
#pragma once

#include "sys/kmem.h"
#include "sys/memlayout.h"
#include "sys/mmu.h"
#include "sys/pmm.h"
#include "sys/vmm.h"

//pgdir_cache.cc
void pgdir_cache_init();
vmm::pde_ptr_t pgdir_entry_alloc();
void pgdir_entry_free(vmm::pde_ptr_t entry);