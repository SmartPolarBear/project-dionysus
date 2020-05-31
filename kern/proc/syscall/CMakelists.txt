cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

add_subdirectory(implements)

target_sources(kernel
    PRIVATE syscall.cc syscall_body.cc syscall_table.cc syscall_entry.S)

