cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

add_subdirectory(syscall)

target_sources(kernel
        PRIVATE thread.S
        PRIVATE thread.cc
        PRIVATE wait_queue.cc
        PRIVATE deadline.cc
        PRIVATE ipc_state.cc
        PRIVATE scheduler_state.cc
        PRIVATE user_stack.cc)

