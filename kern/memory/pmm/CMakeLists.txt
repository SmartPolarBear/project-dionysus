cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

target_sources(kernel
        PRIVATE kmem.cc
        PRIVATE pmm.cc
        PRIVATE buddy_provider.cc
        PRIVATE pmm_old.cc)