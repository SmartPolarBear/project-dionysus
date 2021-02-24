cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

target_sources(kernel
        PRIVATE handle_table.cc
        PRIVATE handle_entry.cc
        PRIVATE kernel_object.cc
        PRIVATE object_manager.cc)

