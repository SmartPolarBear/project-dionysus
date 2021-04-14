cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

target_sources(kernel
        PRIVATE kdebug.cc
        PRIVATE kerror.cc
        PRIVATE kpanic.cc
        PRIVATE backtrace.cc)