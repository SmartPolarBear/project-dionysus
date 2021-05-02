cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

add_subdirectory(fcfs)

add_subdirectory(mlfq)

target_sources(kernel
        PRIVATE scheduler.cc
        PRIVATE scheduler_class.cc)

