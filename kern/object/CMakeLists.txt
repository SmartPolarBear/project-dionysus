cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

target_sources(kernel
        PRIVATE handle.cc
        PRIVATE handle_table.cc
        PRIVATE kernel_object.cc
        PRIVATE object_manager.cc)

