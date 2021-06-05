cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

target_sources(kernel
        PRIVATE address.cc
        PRIVATE gdt.cc
        PRIVATE kmalloc.cc
        PRIVATE page_fault.cc
        PRIVATE paging.cc
        PRIVATE vmm.cc
        PRIVATE vma.cc
        PRIVATE mm.cc
        PRIVATE address_space.cc
        PRIVATE pgdir_cache.cc)