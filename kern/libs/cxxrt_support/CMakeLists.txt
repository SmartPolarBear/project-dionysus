cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

target_sources(kernel
        PRIVATE cxx_symbols.cc
        PRIVATE exception_symbols.cc
        PRIVATE operator_new.cc
        PRIVATE operator_delete.cc)

target_include_directories(kernel PRIVATE include)

# TODO: more elegant way to do this
target_include_directories(arch_amd64 PRIVATE include)