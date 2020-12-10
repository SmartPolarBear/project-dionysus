cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

target_sources(kernel
        PRIVATE cxx_symbols.cc
        PRIVATE operator_new.cc
        PRIVATE operator_delete.cc)

target_include_directories(kernel PRIVATE include)