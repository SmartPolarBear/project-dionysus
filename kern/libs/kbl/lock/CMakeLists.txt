cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

target_sources(kernel
        PRIVATE spinlock.cc
        PRIVATE semaphore.cc
        PRIVATE condition_variable.cc)