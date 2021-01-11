cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

target_sources(kernel
        PRIVATE thread.cc
        PRIVATE thread_current.cc
        PRIVATE wait_queue.cc
        PRIVATE task_state.cc
        PRIVATE thread_dispatcher.cpp)

