cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

add_subdirectory(elf)

add_subdirectory(ipc)

target_sources(kernel
        PRIVATE process.cc)
