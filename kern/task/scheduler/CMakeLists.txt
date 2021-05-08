cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

if (${SCHEDULER} STREQUAL "FCFS")
    add_subdirectory(fcfs)
elseif (${SCHEDULER} STREQUAL "ULE")
    add_subdirectory(ule)
else ()
    message(ERROR "${SCHEDULER} is not a valid scheduler option.")
endif ()

target_sources(kernel
        PRIVATE scheduler.cc
        PRIVATE scheduler_class.cc)

