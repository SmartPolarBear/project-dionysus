cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

target_sources(kernel
        PRIVATE elf.cc
        PRIVATE process.cc
        PRIVATE load_code.cc)
