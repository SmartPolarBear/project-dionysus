cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

target_sources(kernel
        PRIVATE builtin_text_io.cc
        PRIVATE ftoa.cc
        PRIVATE itoa.cc)

target_include_directories(kernel PRIVATE include)