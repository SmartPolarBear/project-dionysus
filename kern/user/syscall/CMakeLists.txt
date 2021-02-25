cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

add_subdirectory(implements)

target_sources(kernel
        PRIVATE syscall.cc
        PRIVATE syscall_body.cc
        PRIVATE syscall_table.cc
        PRIVATE syscall_args.cc
        PRIVATE syscall_entry.S)

