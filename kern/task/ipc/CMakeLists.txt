cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

add_subdirectory(syscall)

target_sources(kernel
        PRIVATE ipc.cc
        PRIVATE endpoint.cc)
